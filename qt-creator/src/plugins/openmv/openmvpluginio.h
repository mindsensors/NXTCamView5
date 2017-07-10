#ifndef OPENMVPLUGINIO_H
#define OPENMVPLUGINIO_H

#include <QtCore>
#include <QtGui>

#include "openmvpluginserialport.h"

#define ATTR_CONTRAST                   0
#define ATTR_BRIGHTNESS                 1
#define ATTR_SATURATION                 2
#define ATTR_GAINCEILING                3

#define TEMPLATE_SAVE_PATH_MAX_LEN      55
#define DESCRIPTOR_SAVE_PATH_MAX_LEN    55

#define FLASH_SECTOR_START              4
#define FLASH_SECTOR_END                11
#define FLASH_SECTOR_ALL_START          1
#define FLASH_SECTOR_ALL_END            11

#define FLASH_WRITE_CHUNK_SIZE          56

#define FLASH_ERASE_DELAY               2000
#define FLASH_WRITE_DELAY               1

class OpenMVPluginIO : public QObject
{
    Q_OBJECT

public:

    explicit OpenMVPluginIO(OpenMVPluginSerialPort *port, QObject *parent = Q_NULLPTR);

    bool getTimeout();
    bool frameSizeDumpQueued() const;
    bool getScriptRunningQueued() const;
    bool getAttributeQueued() const;
    bool getTxBufferQueued() const;

public slots:

    void getFirmwareVersion();
    void frameSizeDump();
    void getArchString();
    void scriptExec(const QByteArray &data);
    void scriptStop();
    void getScriptRunning();
    void templateSave(int x, int y, int w, int h, const QByteArray &path);
    void descriptorSave(int x, int y, int w, int h, const QByteArray &path);
    void getAttribute(int attribute);
    void setAttribute(int attribute, int value);
    void sysReset();
    void fbEnable(bool enable);
    void jpegEnable(bool enabled);
    void getTxBuffer();
    void bootloaderStart();
    void bootloaderReset();
    void flashErase(int sector);
    void flashWrite(const QByteArray &data);
    void close();

public slots: // private

    void command();
    void commandResult(const OpenMVPluginSerialPortCommandResult &commandResult);

signals:

    void firmwareVersion(int major, int minor, int patch);
    void frameBufferData(const QPixmap &data);
    void archString(const QString &arch);
    void scriptExecDone();
    void scriptStopDone();
    void scriptRunning(bool running);
    void templateSaveDone();
    void descriptorSaveDone();
    void attribute(int);
    void setAttrributeDone();
    void sysResetDone();
    void fbEnableDone();
    void jpegEnableDone();
    void printData(const QByteArray &data);
    void gotBootloaderStart(bool ok);
    void bootloaderResetDone(bool ok);
    void flashEraseDone(bool ok);
    void flashWriteDone(bool ok);
    void closeResponse();

private:

    void pasrsePrintData(const QByteArray &data);

    OpenMVPluginSerialPort *m_port;

    QQueue<OpenMVPluginSerialPortCommand> m_postedQueue;
    QQueue<int> m_completionQueue;
    int m_frameSizeW;
    int m_frameSizeH;
    int m_frameSizeBPP;
    QByteArray m_lineBuffer;
    bool m_timeout;
};

#endif // OPENMVPLUGINIO_H
