#include "openmvpluginio.h"

#define USBDBG_FW_VERSION_CPL 0
#define USBDBG_FRAME_SIZE_CPL 1
#define USBDBG_FRAME_DUMP_CPL 2
#define USBDBG_ARCH_STR_CPL 3
#define USBDBG_SCRIPT_EXEC_CPL_0 4
#define USBDBG_SCRIPT_EXEC_CPL_1 5
#define USBDBG_SCRIPT_STOP_CPL 6
#define USBDBG_SCRIPT_RUNNING_CPL 7
#define USBDBG_TEMPLATE_SAVE_CPL_0 8
#define USBDBG_TEMPLATE_SAVE_CPL_1 9
#define USBDBG_DESCRIPTOR_SAVE_CPL_0 10
#define USBDBG_DESCRIPTOR_SAVE_CPL_1 11
#define USBDBG_ATTR_READ_CPL 12
#define USBDBG_ATTR_WRITE_CPL 13
#define USBDBG_SYS_RESET_CPL 14
#define USBDBG_FB_ENABLE_CPL 15
#define USBDBG_JPEG_ENABLE_CPL 16
#define USBDBG_TX_BUF_LEN_CPL 17
#define USBDBG_TX_BUF_CPL 18
#define BOOTLDR_START_CPL 19
#define BOOTLDR_RESET_CPL 20
#define BOOTLDR_ERASE_CPL 21
#define BOOTLDR_WRITE_CPL 22
#define CLOSE_CPL 23

#define IS_JPG(bpp)     ((bpp) >= 3)
#define IS_RGB(bpp)     ((bpp) == 2)
#define IS_GS(bpp)      ((bpp) == 1)
#define IS_BINARY(bpp)  ((bpp) == 0)

static QByteArray byteSwap(QByteArray buffer, bool ok)
{
    if(ok)
    {
        for(int i = 0, j = (buffer.size() / 2) * 2; i < j; i += 2)
        {
            char temp = buffer.data()[i];
            buffer.data()[i] = buffer.data()[i+1];
            buffer.data()[i+1] = temp;
        }
    }

    return buffer;
}

OpenMVPluginIO::OpenMVPluginIO(OpenMVPluginSerialPort *port, QObject *parent) : QObject(parent)
{
    m_port = port;

    connect(m_port, &OpenMVPluginSerialPort::commandResult,
            this, &OpenMVPluginIO::commandResult);

    m_postedQueue = QQueue<OpenMVPluginSerialPortCommand>();
    m_completionQueue = QQueue<int>();
    m_frameSizeW = int();
    m_frameSizeH = int();
    m_frameSizeBPP = int();
    m_lineBuffer = QByteArray();
    m_timeout = bool();
}

void OpenMVPluginIO::command()
{
    if((!m_postedQueue.isEmpty())
    && (!m_completionQueue.isEmpty())
    && (m_postedQueue.size() == m_completionQueue.size()))
    {
        m_port->command(m_postedQueue.dequeue());
    }
}

void OpenMVPluginIO::commandResult(const OpenMVPluginSerialPortCommandResult &commandResult)
{
    if(Q_LIKELY(!m_completionQueue.isEmpty()))
    {
        if(commandResult.m_ok)
        {
            QByteArray data = commandResult.m_data;

            switch(m_completionQueue.head())
            {
                case USBDBG_FW_VERSION_CPL:
                {
                    // The optimizer will mess up the order if executed in emit.
                    int major = deserializeLong(data);
                    int minor = deserializeLong(data);
                    int patch = deserializeLong(data);
                    emit firmwareVersion(major, minor, patch);
                    break;
                }
                case USBDBG_FRAME_SIZE_CPL:
                {
                    int w = deserializeLong(data);
                    int h = deserializeLong(data);
                    int bpp = deserializeLong(data);

                    if(w)
                    {
                        int size = IS_JPG(bpp) ? bpp : ((IS_RGB(bpp) || IS_GS(bpp)) ? (w * h * bpp) : (IS_BINARY(bpp) ? (((w + 31) / 32) * h) : int()));

                        if(size)
                        {
                            QByteArray buffer;
                            serializeByte(buffer, __USBDBG_CMD);
                            serializeByte(buffer, __USBDBG_FRAME_DUMP);
                            serializeLong(buffer, size);
                            m_postedQueue.push_front(OpenMVPluginSerialPortCommand(buffer, size, FRAME_DUMP_START_DELAY, FRAME_DUMP_END_DELAY));
                            m_completionQueue.insert(1, USBDBG_FRAME_DUMP_CPL);
                            m_frameSizeW = w;
                            m_frameSizeH = h;
                            m_frameSizeBPP = bpp;
                        }
                    }

                    break;
                }
                case USBDBG_FRAME_DUMP_CPL:
                {
                    QPixmap pixmap = QPixmap::fromImage(IS_JPG(m_frameSizeBPP)
                    ? QImage::fromData(data, "JPG")
                    : QImage(reinterpret_cast<const uchar *>(byteSwap(data,
                        IS_RGB(m_frameSizeBPP)).constData()), m_frameSizeW, m_frameSizeH, IS_BINARY(m_frameSizeBPP) ? ((m_frameSizeW + 31) / 32) : (m_frameSizeW * m_frameSizeBPP),
                        IS_RGB(m_frameSizeBPP) ? QImage::Format_RGB16 : (IS_GS(m_frameSizeBPP) ? QImage::Format_Grayscale8 : QImage::Format_MonoLSB)));

                    if(pixmap.isNull() && IS_JPG(m_frameSizeBPP))
                    {
                        data = data.mid(1, data.size() - 2);

                        int size = data.size();
                        QByteArray temp;

                        for(int i = 0, j = (size / 4) * 4; i < j; i += 4)
                        {
                            int x = 0;
                            x |= (data.at(i + 0) & 0x3F) << 0;
                            x |= (data.at(i + 1) & 0x3F) << 6;
                            x |= (data.at(i + 2) & 0x3F) << 12;
                            x |= (data.at(i + 3) & 0x3F) << 18;
                            temp.append((x >> 0) & 0xFF);
                            temp.append((x >> 8) & 0xFF);
                            temp.append((x >> 16) & 0xFF);
                        }

                        if((size % 4) == 3) // 2 bytes -> 16-bits -> 24-bits sent
                        {
                            int x = 0;
                            x |= (data.at(size - 3) & 0x3F) << 0;
                            x |= (data.at(size - 2) & 0x3F) << 6;
                            x |= (data.at(size - 1) & 0x0F) << 12;
                            temp.append((x >> 0) & 0xFF);
                            temp.append((x >> 8) & 0xFF);
                        }

                        if((size % 4) == 2) // 1 byte -> 8-bits -> 16-bits sent
                        {
                            int x = 0;
                            x |= (data.at(size - 2) & 0x3F) << 0;
                            x |= (data.at(size - 1) & 0x03) << 6;
                            temp.append((x >> 0) & 0xFF);
                        }

                        pixmap = QPixmap::fromImage(QImage::fromData(temp, "JPG"));
                    }

                    if(!pixmap.isNull())
                    {
                        emit frameBufferData(pixmap);
                    }

                    m_frameSizeW = int();
                    m_frameSizeH = int();
                    m_frameSizeBPP = int();
                    break;
                }
                case USBDBG_ARCH_STR_CPL:
                {
                    emit archString(QString::fromUtf8(data));
                    break;
                }
                case USBDBG_SCRIPT_EXEC_CPL_0:
                {
                    break;
                }
                case USBDBG_SCRIPT_EXEC_CPL_1:
                {
                    emit scriptExecDone();
                    break;
                }
                case USBDBG_SCRIPT_STOP_CPL:
                {
                    emit scriptStopDone();
                    break;
                }
                case USBDBG_SCRIPT_RUNNING_CPL:
                {
                    emit scriptRunning(deserializeLong(data));
                    break;
                }
                case USBDBG_TEMPLATE_SAVE_CPL_0:
                {
                    break;
                }
                case USBDBG_TEMPLATE_SAVE_CPL_1:
                {
                    emit templateSaveDone();
                    break;
                }
                case USBDBG_DESCRIPTOR_SAVE_CPL_0:
                {
                    break;
                }
                case USBDBG_DESCRIPTOR_SAVE_CPL_1:
                {
                    emit descriptorSaveDone();
                    break;
                }
                case USBDBG_ATTR_READ_CPL:
                {
                    emit attribute(deserializeByte(data));
                    break;
                }
                case USBDBG_ATTR_WRITE_CPL:
                {
                    emit setAttrributeDone();
                    break;
                }
                case USBDBG_SYS_RESET_CPL:
                {
                    emit sysResetDone();
                    break;
                }
                case USBDBG_FB_ENABLE_CPL:
                {
                    emit fbEnableDone();
                    break;
                }
                case USBDBG_JPEG_ENABLE_CPL:
                {
                    emit jpegEnableDone();
                    break;
                }
                case USBDBG_TX_BUF_LEN_CPL:
                {
                    int len = deserializeLong(data);

                    if(len)
                    {
                        QByteArray buffer;
                        serializeByte(buffer, __USBDBG_CMD);
                        serializeByte(buffer, __USBDBG_TX_BUF);
                        serializeLong(buffer, len);
                        m_postedQueue.push_front(OpenMVPluginSerialPortCommand(buffer, len, TX_BUF_START_DELAY, TX_BUF_END_DELAY));
                        m_completionQueue.insert(1, USBDBG_TX_BUF_CPL);
                    }
                    else if(m_lineBuffer.size())
                    {
                        emit pasrsePrintData(m_lineBuffer);
                        m_lineBuffer.clear();
                    }

                    break;
                }
                case USBDBG_TX_BUF_CPL:
                {
                    m_lineBuffer.append(data);
                    QByteArrayList list = m_lineBuffer.split('\n');
                    m_lineBuffer = list.takeLast();

                    if(list.size())
                    {
                        emit pasrsePrintData(list.join('\n') + '\n');
                    }

                    break;
                }
                case BOOTLDR_START_CPL:
                {
                    emit gotBootloaderStart(deserializeLong(data) == __BOOTLDR_START);
                    break;
                }
                case BOOTLDR_RESET_CPL:
                {
                    emit bootloaderResetDone(true);
                    break;
                }
                case BOOTLDR_ERASE_CPL:
                {
                    emit flashEraseDone(true);
                    break;
                }
                case BOOTLDR_WRITE_CPL:
                {
                    emit flashWriteDone(true);
                    break;
                }
                case CLOSE_CPL:
                {
                    if(m_lineBuffer.size())
                    {
                        emit pasrsePrintData(m_lineBuffer);
                        m_lineBuffer.clear();
                    }

                    emit closeResponse();
                    break;
                }
            }

            m_completionQueue.dequeue();

            command();
        }
        else
        {
            forever
            {
                switch(m_completionQueue.head())
                {
                    case USBDBG_FW_VERSION_CPL:
                    {
                        emit firmwareVersion(int(), int(), int());
                        break;
                    }
                    case USBDBG_FRAME_SIZE_CPL:
                    {
                        break;
                    }
                    case USBDBG_FRAME_DUMP_CPL:
                    {
                        m_frameSizeW = int();
                        m_frameSizeH = int();
                        m_frameSizeBPP = int();
                        break;
                    }
                    case USBDBG_ARCH_STR_CPL:
                    {
                        emit archString(QString());
                        break;
                    }
                    case USBDBG_SCRIPT_EXEC_CPL_0:
                    {
                        break;
                    }
                    case USBDBG_SCRIPT_EXEC_CPL_1:
                    {
                        emit scriptExecDone();
                        break;
                    }
                    case USBDBG_SCRIPT_STOP_CPL:
                    {
                        emit scriptStopDone();
                        break;
                    }
                    case USBDBG_SCRIPT_RUNNING_CPL:
                    {
                        emit scriptRunning(bool());
                        break;
                    }
                    case USBDBG_TEMPLATE_SAVE_CPL_0:
                    {
                        break;
                    }
                    case USBDBG_TEMPLATE_SAVE_CPL_1:
                    {
                        emit templateSaveDone();
                        break;
                    }
                    case USBDBG_DESCRIPTOR_SAVE_CPL_0:
                    {
                        break;
                    }
                    case USBDBG_DESCRIPTOR_SAVE_CPL_1:
                    {
                        emit descriptorSaveDone();
                        break;
                    }
                    case USBDBG_ATTR_READ_CPL:
                    {
                        emit attribute(int());
                        break;
                    }
                    case USBDBG_ATTR_WRITE_CPL:
                    {
                        emit setAttrributeDone();
                        break;
                    }
                    case USBDBG_SYS_RESET_CPL:
                    {
                        emit sysResetDone();
                        break;
                    }
                    case USBDBG_FB_ENABLE_CPL:
                    {
                        emit fbEnableDone();
                        break;
                    }
                    case USBDBG_JPEG_ENABLE_CPL:
                    {
                        emit jpegEnableDone();
                        break;
                    }
                    case USBDBG_TX_BUF_LEN_CPL:
                    {
                        if(m_lineBuffer.size())
                        {
                            emit pasrsePrintData(m_lineBuffer);
                            m_lineBuffer.clear();
                        }

                        break;
                    }
                    case USBDBG_TX_BUF_CPL:
                    {
                        if(m_lineBuffer.size())
                        {
                            emit pasrsePrintData(m_lineBuffer);
                            m_lineBuffer.clear();
                        }

                        break;
                    }
                    case BOOTLDR_START_CPL:
                    {
                        emit gotBootloaderStart(false);
                        break;
                    }
                    case BOOTLDR_RESET_CPL:
                    {
                        emit bootloaderResetDone(false);
                        break;
                    }
                    case BOOTLDR_ERASE_CPL:
                    {
                        emit flashEraseDone(false);
                        break;
                    }
                    case BOOTLDR_WRITE_CPL:
                    {
                        emit flashWriteDone(false);
                        break;
                    }
                    case CLOSE_CPL:
                    {
                        if(m_lineBuffer.size())
                        {
                            emit pasrsePrintData(m_lineBuffer);
                            m_lineBuffer.clear();
                        }

                        emit closeResponse();
                        break;
                    }
                }

                m_completionQueue.dequeue();

                if((!m_postedQueue.isEmpty())
                && (!m_completionQueue.isEmpty())
                && (m_postedQueue.size() == m_completionQueue.size()))
                {
                    m_postedQueue.dequeue();
                }
                else
                {
                    break;
                }
            }

            m_timeout = true;
        }
    }
}

bool OpenMVPluginIO::getTimeout()
{
    bool timeout = m_timeout;
    m_timeout = false;
    return timeout;
}

bool OpenMVPluginIO::frameSizeDumpQueued() const
{
    return m_completionQueue.contains(USBDBG_FRAME_SIZE_CPL) ||
           m_completionQueue.contains(USBDBG_FRAME_DUMP_CPL);
}

bool OpenMVPluginIO::getScriptRunningQueued() const
{
    return m_completionQueue.contains(USBDBG_SCRIPT_RUNNING_CPL);
}

bool OpenMVPluginIO::getAttributeQueued() const
{
    return m_completionQueue.contains(USBDBG_ATTR_READ_CPL);
}

bool OpenMVPluginIO::getTxBufferQueued() const
{
    return m_completionQueue.contains(USBDBG_TX_BUF_LEN_CPL) ||
           m_completionQueue.contains(USBDBG_TX_BUF_CPL);
}

void OpenMVPluginIO::getFirmwareVersion()
{
    QByteArray buffer;
    serializeByte(buffer, __USBDBG_CMD);
    serializeByte(buffer, __USBDBG_FW_VERSION);
    serializeLong(buffer, FW_VERSION_RESPONSE_LEN);
    m_postedQueue.enqueue(OpenMVPluginSerialPortCommand(buffer, FW_VERSION_RESPONSE_LEN, FW_VERSION_START_DELAY, FW_VERSION_END_DELAY));
    m_completionQueue.enqueue(USBDBG_FW_VERSION_CPL);
    command();
}

void OpenMVPluginIO::frameSizeDump()
{
    QByteArray buffer;
    serializeByte(buffer, __USBDBG_CMD);
    serializeByte(buffer, __USBDBG_FRAME_SIZE);
    serializeLong(buffer, FRAME_SIZE_RESPONSE_LEN);
    m_postedQueue.enqueue(OpenMVPluginSerialPortCommand(buffer, FRAME_SIZE_RESPONSE_LEN, FRAME_SIZE_START_DELAY, FRAME_SIZE_END_DELAY));
    m_completionQueue.enqueue(USBDBG_FRAME_SIZE_CPL);
    command();
}

void OpenMVPluginIO::getArchString()
{
    QByteArray buffer;
    serializeByte(buffer, __USBDBG_CMD);
    serializeByte(buffer, __USBDBG_ARCH_STR);
    serializeLong(buffer, ARCH_STR_RESPONSE_LEN);
    m_postedQueue.enqueue(OpenMVPluginSerialPortCommand(buffer, ARCH_STR_RESPONSE_LEN, ARCH_STR_START_DELAY, ARCH_STR_END_DELAY));
    m_completionQueue.enqueue(USBDBG_ARCH_STR_CPL);
    command();
}

void OpenMVPluginIO::scriptExec(const QByteArray &data)
{
    QByteArray buffer, script = (data.size() % TABOO_PACKET_SIZE) ? data : (data + '\n');
    serializeByte(buffer, __USBDBG_CMD);
    serializeByte(buffer, __USBDBG_SCRIPT_EXEC);
    serializeLong(buffer, script.size());
    m_postedQueue.enqueue(OpenMVPluginSerialPortCommand(buffer, int(), SCRIPT_EXEC_START_DELAY, SCRIPT_EXEC_END_DELAY));
    m_completionQueue.enqueue(USBDBG_SCRIPT_EXEC_CPL_0);
    command();
    m_postedQueue.enqueue(OpenMVPluginSerialPortCommand(script, int(), SCRIPT_EXEC_2_START_DELAY, SCRIPT_EXEC_2_END_DELAY));
    m_completionQueue.enqueue(USBDBG_SCRIPT_EXEC_CPL_1);
    command();
}

void OpenMVPluginIO::scriptStop()
{
    QByteArray buffer;
    serializeByte(buffer, __USBDBG_CMD);
    serializeByte(buffer, __USBDBG_SCRIPT_STOP);
    serializeLong(buffer, int());
    m_postedQueue.enqueue(OpenMVPluginSerialPortCommand(buffer, int(), SCRIPT_STOP_START_DELAY, SCRIPT_STOP_END_DELAY));
    m_completionQueue.enqueue(USBDBG_SCRIPT_STOP_CPL);
    command();
}

void OpenMVPluginIO::getScriptRunning()
{
    QByteArray buffer;
    serializeByte(buffer, __USBDBG_CMD);
    serializeByte(buffer, __USBDBG_SCRIPT_RUNNING);
    serializeLong(buffer, SCRIPT_RUNNING_RESPONSE_LEN);
    m_postedQueue.enqueue(OpenMVPluginSerialPortCommand(buffer, SCRIPT_RUNNING_RESPONSE_LEN, SCRIPT_RUNNING_START_DELAY, SCRIPT_RUNNING_END_DELAY));
    m_completionQueue.enqueue(USBDBG_SCRIPT_RUNNING_CPL);
    command();
}

void OpenMVPluginIO::templateSave(int x, int y, int w, int h, const QByteArray &path)
{
    QByteArray buffer;
    serializeByte(buffer, __USBDBG_CMD);
    serializeByte(buffer, __USBDBG_TEMPLATE_SAVE);
    serializeLong(buffer, 2 + 2 + 2 + 2 + path.size());
    m_postedQueue.enqueue(OpenMVPluginSerialPortCommand(buffer, int(), TEMPLATE_SAVE_START_DELAY, TEMPLATE_SAVE_END_DELAY));
    m_completionQueue.enqueue(USBDBG_TEMPLATE_SAVE_CPL_0);
    command();
    buffer.clear();
    serializeWord(buffer, x);
    serializeWord(buffer, y);
    serializeWord(buffer, w);
    serializeWord(buffer, h);
    m_postedQueue.enqueue(OpenMVPluginSerialPortCommand(buffer + path, int(), TEMPLATE_SAVE_2_START_DELAY, TEMPLATE_SAVE_2_END_DELAY));
    m_completionQueue.enqueue(USBDBG_TEMPLATE_SAVE_CPL_1);
    command();
}

void OpenMVPluginIO::descriptorSave(int x, int y, int w, int h, const QByteArray &path)
{
    QByteArray buffer;
    serializeByte(buffer, __USBDBG_CMD);
    serializeByte(buffer, __USBDBG_DESCRIPTOR_SAVE);
    serializeLong(buffer, 2 + 2 + 2 + 2 + path.size());
    m_postedQueue.enqueue(OpenMVPluginSerialPortCommand(buffer, int(), DESCRIPTOR_SAVE_START_DELAY, DESCRIPTOR_SAVE_END_DELAY));
    m_completionQueue.enqueue(USBDBG_DESCRIPTOR_SAVE_CPL_0);
    command();
    buffer.clear();
    serializeWord(buffer, x);
    serializeWord(buffer, y);
    serializeWord(buffer, w);
    serializeWord(buffer, h);
    m_postedQueue.enqueue(OpenMVPluginSerialPortCommand(buffer + path, int(), DESCRIPTOR_SAVE_2_START_DELAY, DESCRIPTOR_SAVE_2_END_DELAY));
    m_completionQueue.enqueue(USBDBG_DESCRIPTOR_SAVE_CPL_1);
    command();
}

void OpenMVPluginIO::getAttribute(int attribute)
{
    QByteArray buffer;
    serializeByte(buffer, __USBDBG_CMD);
    serializeByte(buffer, __USBDBG_ATTR_READ);
    serializeLong(buffer, ATTR_READ_RESPONSE_LEN);
    serializeWord(buffer, attribute);
    m_postedQueue.enqueue(OpenMVPluginSerialPortCommand(buffer, ATTR_READ_RESPONSE_LEN, ATTR_READ_START_DELAY, ATTR_READ_END_DELAY));
    m_completionQueue.enqueue(USBDBG_ATTR_READ_CPL);
    command();
}

void OpenMVPluginIO::setAttribute(int attribute, int value)
{
    QByteArray buffer;
    serializeByte(buffer, __USBDBG_CMD);
    serializeByte(buffer, __USBDBG_ATTR_WRITE);
    serializeLong(buffer, int());
    serializeWord(buffer, attribute);
    serializeWord(buffer, value);
    m_postedQueue.enqueue(OpenMVPluginSerialPortCommand(buffer, int(), ATTR_WRITE_START_DELAY, ATTR_WRITE_END_DELAY));
    m_completionQueue.enqueue(USBDBG_ATTR_WRITE_CPL);
    command();
}

void OpenMVPluginIO::sysReset()
{
    QByteArray buffer;
    serializeByte(buffer, __USBDBG_CMD);
    serializeByte(buffer, __USBDBG_SYS_RESET);
    serializeLong(buffer, int());
    m_postedQueue.enqueue(OpenMVPluginSerialPortCommand(buffer, int(), SYS_RESET_START_DELAY, SYS_RESET_END_DELAY));
    m_completionQueue.enqueue(USBDBG_SYS_RESET_CPL);
    command();
}

void OpenMVPluginIO::fbEnable(bool enabled)
{
    QByteArray buffer;
    serializeByte(buffer, __USBDBG_CMD);
    serializeByte(buffer, __USBDBG_FB_ENABLE);
    serializeLong(buffer, int());
    serializeWord(buffer, enabled ? true : false);
    m_postedQueue.enqueue(OpenMVPluginSerialPortCommand(buffer, int(), FB_ENABLE_START_DELAY, FB_ENABLE_END_DELAY));
    m_completionQueue.enqueue(USBDBG_FB_ENABLE_CPL);
    command();
}

void OpenMVPluginIO::jpegEnable(bool enabled)
{
    Q_UNUSED(enabled)

//  QByteArray buffer;
//  serializeByte(buffer, __USBDBG_CMD);
//  serializeByte(buffer, __USBDBG_JPEG_ENABLE);
//  serializeLong(buffer, int());
//  serializeWord(buffer, enabled ? true : false);
//  m_postedQueue.enqueue(OpenMVPluginSerialPortCommand(buffer, int(), JPEG_ENABLE_START_DELAY, JPEG_ENABLE_END_DELAY));
//  m_completionQueue.enqueue(USBDBG_JPEG_ENABLE_CPL);
//  command();
}

void OpenMVPluginIO::getTxBuffer()
{
    QByteArray buffer;
    serializeByte(buffer, __USBDBG_CMD);
    serializeByte(buffer, __USBDBG_TX_BUF_LEN);
    serializeLong(buffer, TX_BUF_LEN_RESPONSE_LEN);
    m_postedQueue.enqueue(OpenMVPluginSerialPortCommand(buffer, TX_BUF_LEN_RESPONSE_LEN, TX_BUF_LEN_START_DELAY, TX_BUF_LEN_END_DELAY));
    m_completionQueue.enqueue(USBDBG_TX_BUF_LEN_CPL);
    command();
}

void OpenMVPluginIO::bootloaderStart()
{
    QByteArray buffer;
    serializeLong(buffer, __BOOTLDR_START);
    m_postedQueue.enqueue(OpenMVPluginSerialPortCommand(buffer, BOOTLDR_START_RESPONSE_LEN, BOOTLDR_START_START_DELAY, BOOTLDR_START_END_DELAY));
    m_completionQueue.enqueue(BOOTLDR_START_CPL);
    command();
}

void OpenMVPluginIO::bootloaderReset()
{
    QByteArray buffer;
    serializeLong(buffer, __BOOTLDR_RESET);
    m_postedQueue.enqueue(OpenMVPluginSerialPortCommand(buffer, int(), BOOTLDR_RESET_START_DELAY, BOOTLDR_RESET_END_DELAY));
    m_completionQueue.enqueue(BOOTLDR_RESET_CPL);
    command();
}

void OpenMVPluginIO::flashErase(int sector)
{
    QByteArray buffer;
    serializeLong(buffer, __BOOTLDR_ERASE);
    serializeLong(buffer, sector);
    m_postedQueue.enqueue(OpenMVPluginSerialPortCommand(buffer, int(), BOOTLDR_ERASE_START_DELAY, BOOTLDR_ERASE_END_DELAY));
    m_completionQueue.enqueue(BOOTLDR_ERASE_CPL);
    command();
}

void OpenMVPluginIO::flashWrite(const QByteArray &data)
{
    QByteArray buffer;
    serializeLong(buffer, __BOOTLDR_WRITE);
    m_postedQueue.enqueue(OpenMVPluginSerialPortCommand(buffer + data, int(), BOOTLDR_WRITE_START_DELAY, BOOTLDR_WRITE_END_DELAY));
    m_completionQueue.enqueue(BOOTLDR_WRITE_CPL);
    command();
}

void OpenMVPluginIO::close()
{
    m_postedQueue.enqueue(OpenMVPluginSerialPortCommand(QByteArray(), int(), int(), int()));
    m_completionQueue.enqueue(CLOSE_CPL);
    command();
}

void OpenMVPluginIO::pasrsePrintData(const QByteArray &data)
{
    enum
    {
        ASCII,
        UTF_8,
        EXIT_0,
        EXIT_1
    }
    stateMachine = ASCII;

    QByteArray shiftReg = QByteArray();

    QByteArray buffer;

    for(int i = 0, j = data.size(); i < j; i++)
    {
        if((stateMachine == UTF_8) && ((data.at(i) & 0xC0) != 0x80))
        {
            stateMachine = ASCII;
        }

        if((stateMachine == EXIT_0) && ((data.at(i) & 0xFF) != 0x00))
        {
            stateMachine = ASCII;
        }

        switch(stateMachine)
        {
            case ASCII:
            {
                if(((data.at(i) & 0xE0) == 0xC0)
                || ((data.at(i) & 0xF0) == 0xE0)
                || ((data.at(i) & 0xF8) == 0xF0)
                || ((data.at(i) & 0xFC) == 0xF8)
                || ((data.at(i) & 0xFE) == 0xFC)) // UTF_8
                {
                    shiftReg.clear();

                    stateMachine = UTF_8;
                }
                else if((data.at(i) & 0xFF) == 0xFF)
                {
                    stateMachine = EXIT_0;
                }
                else if((data.at(i) & 0xC0) == 0x80)
                {
                    // Nothing for now.
                }
                else if((data.at(i) & 0xFF) == 0xFE)
                {
                    // Nothing for now.
                }
                else if((data.at(i) & 0x80) == 0x00) // ASCII
                {
                    buffer.append(data.at(i));
                }

                break;
            }

            case UTF_8:
            {
                if((((shiftReg.at(0) & 0xE0) == 0xC0) && (shiftReg.size() == 1))
                || (((shiftReg.at(0) & 0xF0) == 0xE0) && (shiftReg.size() == 2))
                || (((shiftReg.at(0) & 0xF8) == 0xF0) && (shiftReg.size() == 3))
                || (((shiftReg.at(0) & 0xFC) == 0xF8) && (shiftReg.size() == 4))
                || (((shiftReg.at(0) & 0xFE) == 0xFC) && (shiftReg.size() == 5)))
                {
                    buffer.append(shiftReg + data.at(i));

                    stateMachine = ASCII;
                }

                break;
            }

            case EXIT_0:
            {
                stateMachine = EXIT_1;

                break;
            }

            case EXIT_1:
            {
                stateMachine = ASCII;

                break;
            }
        }

        shiftReg = shiftReg.append(data.at(i)).right(5);
    }

    emit printData(buffer);
}
