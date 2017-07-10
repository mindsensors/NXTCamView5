#include "openmvpluginserialport.h"

#define OPENMVCAM_BAUD_RATE 12000000
#define OPENMVCAM_BAUD_RATE_2 921600

#define WRITE_TIMEOUT 3000
#define READ_TIMEOUT 5000
#define READ_STALL_TIMEOUT 1000
#define BOOTLOADER_WRITE_TIMEOUT 6
#define BOOTLOADER_READ_TIMEOUT 10
#define BOOTLOADER_READ_STALL_TIMEOUT 2

void serializeByte(QByteArray &buffer, int value) // LittleEndian
{
    buffer.append(reinterpret_cast<const char *>(&value), 1);
}

void serializeWord(QByteArray &buffer, int value) // LittleEndian
{
    buffer.append(reinterpret_cast<const char *>(&value), 2);
}

void serializeLong(QByteArray &buffer, int value) // LittleEndian
{
    buffer.append(reinterpret_cast<const char *>(&value), 4);
}

int deserializeByte(QByteArray &buffer) // LittleEndian
{
    int r = int();
    memcpy(&r, buffer.data(), 1);
    buffer = buffer.mid(1);
    return r;
}

int deserializeWord(QByteArray &buffer) // LittleEndian
{
    int r = int();
    memcpy(&r, buffer.data(), 2);
    buffer = buffer.mid(2);
    return r;
}

int deserializeLong(QByteArray &buffer) // LittleEndian
{
    int r = int();
    memcpy(&r, buffer.data(), 4);
    buffer = buffer.mid(4);
    return r;
}

OpenMVPluginSerialPort_private::OpenMVPluginSerialPort_private(QObject *parent) : QObject(parent)
{
    m_port = Q_NULLPTR;
    m_bootloaderStop = false;
}

void OpenMVPluginSerialPort_private::open(const QString &portName)
{
    if(m_port)
    {
        delete m_port;
    }

    m_port = new QSerialPort(portName, this);

    if((!m_port->setBaudRate(OPENMVCAM_BAUD_RATE))
    || (!m_port->open(QIODevice::ReadWrite)))
    {
        delete m_port;
        m_port = new QSerialPort(portName, this);

        if((!m_port->setBaudRate(OPENMVCAM_BAUD_RATE_2))
        || (!m_port->open(QIODevice::ReadWrite)))
        {
            emit openResult(m_port->errorString());
            delete m_port;
            m_port = Q_NULLPTR;
        }
    }

    if(m_port)
    {
        emit openResult(QString());
    }
}

void OpenMVPluginSerialPort_private::write(const QByteArray &data, int startWait, int stopWait, int timeout)
{
    if(m_port)
    {
        if(startWait)
        {
            QThread::msleep(startWait);
        }

        m_port->clearError();

        if((m_port->write(data) != data.size()) || (!m_port->flush()))
        {
            delete m_port;
            m_port = Q_NULLPTR;
        }
        else
        {
            QElapsedTimer elaspedTimer;
            elaspedTimer.start();

            while(m_port->bytesToWrite())
            {
                m_port->waitForBytesWritten(1);

                if(m_port->bytesToWrite() && elaspedTimer.hasExpired(timeout))
                {
                    break;
                }
            }

            if(m_port->bytesToWrite())
            {
                delete m_port;
                m_port = Q_NULLPTR;
            }
            else if(stopWait)
            {
                QThread::msleep(stopWait);
            }
        }
    }
}

void OpenMVPluginSerialPort_private::command(const OpenMVPluginSerialPortCommand &command)
{
    if(command.m_data.isEmpty())
    {
        if(m_port)
        {
            delete m_port;
            m_port = Q_NULLPTR;
        }

        emit commandResult(OpenMVPluginSerialPortCommandResult(true, QByteArray()));
    }
    else if(m_port)
    {
        write(command.m_data, command.m_startWait, command.m_endWait, WRITE_TIMEOUT);

        if((!m_port) || (!command.m_responseLen))
        {
            emit commandResult(OpenMVPluginSerialPortCommandResult(m_port, QByteArray()));
        }
        else
        {
            QByteArray response;
            int responseLen = command.m_responseLen;
            QElapsedTimer elaspedTimer;
            QElapsedTimer elaspedTimer2;
            elaspedTimer.start();
            elaspedTimer2.start();

            do
            {
                m_port->waitForReadyRead(1);
                response.append(m_port->readAll());

                if((response.size() < responseLen) && elaspedTimer2.hasExpired(READ_STALL_TIMEOUT))
                {
                    QByteArray data;
                    serializeByte(data, __USBDBG_CMD);
                    serializeByte(data, __USBDBG_SCRIPT_RUNNING);
                    serializeLong(data, SCRIPT_RUNNING_RESPONSE_LEN);
                    write(data, SCRIPT_RUNNING_START_DELAY, SCRIPT_RUNNING_END_DELAY, WRITE_TIMEOUT);

                    if(m_port)
                    {
                        responseLen += SCRIPT_RUNNING_RESPONSE_LEN;
                        elaspedTimer2.restart();
                    }
                    else
                    {
                        break;
                    }
                }
            }
            while((response.size() < responseLen) && (!elaspedTimer.hasExpired(READ_TIMEOUT)));

            if(response.size() >= responseLen)
            {
                emit commandResult(OpenMVPluginSerialPortCommandResult(true, response.left(command.m_responseLen)));
            }
            else
            {
                if(m_port)
                {
                    delete m_port;
                    m_port = Q_NULLPTR;
                }

                emit commandResult(OpenMVPluginSerialPortCommandResult(false, QByteArray()));
            }
        }
    }
    else
    {
        emit commandResult(OpenMVPluginSerialPortCommandResult(false, QByteArray()));
    }
}

void OpenMVPluginSerialPort_private::bootloaderStart(const QString &selectedPort)
{
    if(m_port)
    {
        QByteArray buffer;
        serializeByte(buffer, __USBDBG_CMD);
        serializeByte(buffer, __USBDBG_SYS_RESET);
        serializeLong(buffer, int());
        write(buffer, SYS_RESET_START_DELAY, SYS_RESET_END_DELAY, WRITE_TIMEOUT);

        if(m_port)
        {
            delete m_port;
            m_port = Q_NULLPTR;
        }
    }

    forever
    {
        QStringList stringList;

        foreach(QSerialPortInfo port, QSerialPortInfo::availablePorts())
        {
            if(port.hasVendorIdentifier() && (port.vendorIdentifier() == OPENMVCAM_VID)
            && port.hasProductIdentifier() &&
            (port.productIdentifier() == OPENMVCAM_PID) ||
            (port.productIdentifier() == NXTCAM_PID))
            {
                stringList.append(port.portName());
            }
        }

        if(Utils::HostOsInfo::isMacHost())
        {
            stringList = stringList.filter(QStringLiteral("cu"), Qt::CaseInsensitive);
        }

        if(!stringList.isEmpty())
        {
            const QString portName = ((!selectedPort.isEmpty()) && stringList.contains(selectedPort)) ? selectedPort : stringList.first();

            if(Q_UNLIKELY(m_port))
            {
                delete m_port;
            }

            m_port = new QSerialPort(portName, this);

            if((!m_port->setBaudRate(OPENMVCAM_BAUD_RATE))
            || (!m_port->open(QIODevice::ReadWrite)))
            {
                delete m_port;
                m_port = new QSerialPort(portName, this);

                if((!m_port->setBaudRate(OPENMVCAM_BAUD_RATE_2))
                || (!m_port->open(QIODevice::ReadWrite)))
                {
                    delete m_port;
                    m_port = Q_NULLPTR;
                }
            }

            if(m_port)
            {
                QByteArray buffer;
                serializeLong(buffer, __BOOTLDR_START);
                write(buffer, BOOTLDR_START_START_DELAY, BOOTLDR_START_END_DELAY, BOOTLOADER_WRITE_TIMEOUT);

                if(m_port)
                {
                    QByteArray response;
                    int responseLen = BOOTLDR_START_RESPONSE_LEN;
                    QElapsedTimer elaspedTimer;
                    QElapsedTimer elaspedTimer2;
                    elaspedTimer.start();
                    elaspedTimer2.start();

                    do
                    {
                        m_port->waitForReadyRead(1);
                        response.append(m_port->readAll());

                        if((response.size() < responseLen) && elaspedTimer2.hasExpired(BOOTLOADER_READ_STALL_TIMEOUT))
                        {
                            QByteArray data;
                            serializeLong(data, __BOOTLDR_START);
                            write(data, BOOTLDR_START_START_DELAY, BOOTLDR_START_END_DELAY, BOOTLOADER_WRITE_TIMEOUT);

                            if(m_port)
                            {
                                responseLen += BOOTLDR_START_RESPONSE_LEN;
                                elaspedTimer2.restart();
                            }
                            else
                            {
                                break;
                            }
                        }
                    }
                    while((response.size() < responseLen) && (!elaspedTimer.hasExpired(BOOTLOADER_READ_TIMEOUT)));

                    if((response.size() >= responseLen) && (deserializeLong(response) == __BOOTLDR_START))
                    {
                        emit bootloaderStartResponse(true);
                        return;
                    }
                    else
                    {
                        if(m_port)
                        {
                            delete m_port;
                            m_port = Q_NULLPTR;
                        }
                    }
                }
            }
        }

        QCoreApplication::processEvents();

        if(m_bootloaderStop)
        {
            emit bootloaderStartResponse(false);
            return;
        }
    }
}

void OpenMVPluginSerialPort_private::bootloaderStop()
{
    m_bootloaderStop = true;
    emit bootloaderStopResponse();
}

void OpenMVPluginSerialPort_private::bootloaderReset()
{
    m_bootloaderStop = false;
    emit bootloaderResetResponse();
}

OpenMVPluginSerialPort::OpenMVPluginSerialPort(QObject *parent) : QObject(parent)
{
    QThread *thread = new QThread;
    OpenMVPluginSerialPort_private* port = new OpenMVPluginSerialPort_private;
    port->moveToThread(thread);

    connect(this, &OpenMVPluginSerialPort::open,
            port, &OpenMVPluginSerialPort_private::open);

    connect(port, &OpenMVPluginSerialPort_private::openResult,
            this, &OpenMVPluginSerialPort::openResult);

    connect(this, &OpenMVPluginSerialPort::command,
            port, &OpenMVPluginSerialPort_private::command);

    connect(port, &OpenMVPluginSerialPort_private::commandResult,
            this, &OpenMVPluginSerialPort::commandResult);

    connect(this, &OpenMVPluginSerialPort::bootloaderStart,
            port, &OpenMVPluginSerialPort_private::bootloaderStart);

    connect(this, &OpenMVPluginSerialPort::bootloaderStop,
            port, &OpenMVPluginSerialPort_private::bootloaderStop);

    connect(this, &OpenMVPluginSerialPort::bootloaderReset,
            port, &OpenMVPluginSerialPort_private::bootloaderReset);

    connect(port, &OpenMVPluginSerialPort_private::bootloaderStartResponse,
            this, &OpenMVPluginSerialPort::bootloaderStartResponse);

    connect(port, &OpenMVPluginSerialPort_private::bootloaderStopResponse,
            this, &OpenMVPluginSerialPort::bootloaderStopResponse);

    connect(port, &OpenMVPluginSerialPort_private::bootloaderResetResponse,
            this, &OpenMVPluginSerialPort::bootloaderResetResponse);

    connect(this, &OpenMVPluginSerialPort::destroyed,
            port, &OpenMVPluginSerialPort_private::deleteLater);

    connect(port, &OpenMVPluginSerialPort_private::destroyed,
            thread, &QThread::quit);

    connect(thread, &QThread::finished,
            thread, &QThread::deleteLater);

    thread->start();
}
