#ifndef OPENMVPLUGINSERIALPORT_H
#define OPENMVPLUGINSERIALPORT_H

#include <QtCore>
#include <QtSerialPort>

#include <utils/hostosinfo.h>

#define OPENMVCAM_VID 0x1209
#define OPENMVCAM_PID 0xABD1

#define NXTCAM_VID 0x1209
#define NXTCAM_PID 0x4D53

#define TABOO_PACKET_SIZE 64

///////////////////////////////////////////////////////////////////////////////

#define __USBDBG_CMD                    0x30
#define __USBDBG_FW_VERSION             0x80
#define __USBDBG_FRAME_SIZE             0x81
#define __USBDBG_FRAME_DUMP             0x82
#define __USBDBG_ARCH_STR               0x83
#define __USBDBG_SCRIPT_EXEC            0x05
#define __USBDBG_SCRIPT_STOP            0x06
#define __USBDBG_SCRIPT_RUNNING         0x87
#define __USBDBG_TEMPLATE_SAVE          0x08
#define __USBDBG_DESCRIPTOR_SAVE        0x09
#define __USBDBG_ATTR_READ              0x8A
#define __USBDBG_ATTR_WRITE             0x0B
#define __USBDBG_SYS_RESET              0x0C
#define __USBDBG_FB_ENABLE              0x0D
#define __USBDBG_JPEG_ENABLE            0x0E
#define __USBDBG_TX_BUF_LEN             0x8E
#define __USBDBG_TX_BUF                 0x8F

#define __BOOTLDR_START                 static_cast<int>(0xABCD0001)
#define __BOOTLDR_RESET                 static_cast<int>(0xABCD0002)
#define __BOOTLDR_ERASE                 static_cast<int>(0xABCD0004)
#define __BOOTLDR_WRITE                 static_cast<int>(0xABCD0008)

#define FW_VERSION_RESPONSE_LEN         12
#define ARCH_STR_RESPONSE_LEN           64
#define FRAME_SIZE_RESPONSE_LEN         12
#define SCRIPT_RUNNING_RESPONSE_LEN     4
#define ATTR_READ_RESPONSE_LEN          1
#define TX_BUF_LEN_RESPONSE_LEN         4

#define BOOTLDR_START_RESPONSE_LEN      4

#define FW_VERSION_START_DELAY          100
#define FW_VERSION_END_DELAY            0
#define FRAME_SIZE_START_DELAY          0
#define FRAME_SIZE_END_DELAY            0
#define FRAME_DUMP_START_DELAY          0
#define FRAME_DUMP_END_DELAY            0
#define ARCH_STR_START_DELAY            0
#define ARCH_STR_END_DELAY              0
#define SCRIPT_EXEC_START_DELAY         0
#define SCRIPT_EXEC_END_DELAY           0
#define SCRIPT_EXEC_2_START_DELAY       0
#define SCRIPT_EXEC_2_END_DELAY         0
#define SCRIPT_STOP_START_DELAY         50
#define SCRIPT_STOP_END_DELAY           50
#define SCRIPT_RUNNING_START_DELAY      0
#define SCRIPT_RUNNING_END_DELAY        0
#define TEMPLATE_SAVE_START_DELAY       0
#define TEMPLATE_SAVE_END_DELAY         0
#define TEMPLATE_SAVE_2_START_DELAY     0
#define TEMPLATE_SAVE_2_END_DELAY       0
#define DESCRIPTOR_SAVE_START_DELAY     0
#define DESCRIPTOR_SAVE_END_DELAY       0
#define DESCRIPTOR_SAVE_2_START_DELAY   0
#define DESCRIPTOR_SAVE_2_END_DELAY     0
#define ATTR_READ_START_DELAY           0
#define ATTR_READ_END_DELAY             0
#define ATTR_WRITE_START_DELAY          0
#define ATTR_WRITE_END_DELAY            0
#define SYS_RESET_START_DELAY           0
#define SYS_RESET_END_DELAY             0
#define FB_ENABLE_START_DELAY           0
#define FB_ENABLE_END_DELAY             0
#define JPEG_ENABLE_START_DELAY         0
#define JPEG_ENABLE_END_DELAY           0
#define TX_BUF_LEN_START_DELAY          0
#define TX_BUF_LEN_END_DELAY            0
#define TX_BUF_START_DELAY              0
#define TX_BUF_END_DELAY                0

#define BOOTLDR_START_START_DELAY       0
#define BOOTLDR_START_END_DELAY         0
#define BOOTLDR_RESET_START_DELAY       0
#define BOOTLDR_RESET_END_DELAY         0
#define BOOTLDR_ERASE_START_DELAY       0
#define BOOTLDR_ERASE_END_DELAY         0
#define BOOTLDR_WRITE_START_DELAY       0
#define BOOTLDR_WRITE_END_DELAY         0

///////////////////////////////////////////////////////////////////////////////

void serializeByte(QByteArray &buffer, int value); // LittleEndian
void serializeWord(QByteArray &buffer, int value); // LittleEndian
void serializeLong(QByteArray &buffer, int value); // LittleEndian

int deserializeByte(QByteArray &buffer); // LittleEndian
int deserializeWord(QByteArray &buffer); // LittleEndian
int deserializeLong(QByteArray &buffer); // LittleEndian

class OpenMVPluginSerialPortCommand
{
public:
    explicit OpenMVPluginSerialPortCommand(const QByteArray &data = QByteArray(), int responseLen = int(), int startWait = int(), int endWait = int()) :
        m_data(data), m_responseLen(responseLen), m_startWait(startWait), m_endWait(endWait) { }
    QByteArray m_data;
    int m_responseLen;
    int m_startWait; // in ms
    int m_endWait; // in ms
};

class OpenMVPluginSerialPortCommandResult
{
public:
    explicit OpenMVPluginSerialPortCommandResult(bool ok = bool(), const QByteArray &data = QByteArray()) :
        m_ok(ok), m_data(data) { }
    bool m_ok;
    QByteArray m_data;
};

class OpenMVPluginSerialPort_private : public QObject
{
    Q_OBJECT

public:

    explicit OpenMVPluginSerialPort_private(QObject *parent = Q_NULLPTR);

public slots:

    void open(const QString &portName);
    void command(const OpenMVPluginSerialPortCommand &command);

    void bootloaderStart(const QString &selectedPort);
    void bootloaderStop();
    void bootloaderReset();

signals:

    void openResult(const QString &errorMessage);
    void commandResult(const OpenMVPluginSerialPortCommandResult &commandResult);

    void bootloaderStartResponse(bool ok);
    void bootloaderStopResponse();
    void bootloaderResetResponse();

private:

    void write(const QByteArray &data, int startWait, int stopWait, int timeout);

    QSerialPort *m_port;
    bool m_bootloaderStop;
};

class OpenMVPluginSerialPort : public QObject
{
    Q_OBJECT

public:

    explicit OpenMVPluginSerialPort(QObject *parent = Q_NULLPTR);

signals:

    void open(const QString &portName);
    void openResult(const QString &errorMessage);

    void command(const OpenMVPluginSerialPortCommand &command);
    void commandResult(const OpenMVPluginSerialPortCommandResult &commandResult);

    void bootloaderStart(const QString &selectedPort);
    void bootloaderStop();
    void bootloaderReset();

    void bootloaderStartResponse(bool ok);
    void bootloaderStopResponse();
    void bootloaderResetResponse();
};

#endif // OPENMVPLUGINSERIALPORT_H
