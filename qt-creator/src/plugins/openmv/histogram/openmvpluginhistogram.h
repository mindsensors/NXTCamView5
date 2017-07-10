#ifndef OPENMVPLUGINHISTOGRAM_H
#define OPENMVPLUGINHISTOGRAM_H

#include <QtCore>
#include <QtGui>
#include <QtWidgets>
#include <time.h>

#include "../qcustomplot/qcustomplot.h"
#include "../openmvpluginio.h"

#define RGB_COLOR_SPACE 0
#define GRAYSCALE_COLOR_SPACE 1
#define LAB_COLOR_SPACE 2
#define YUV_COLOR_SPACE 3

namespace Ui
{
    class OpenMVPluginHistogram;
}

class OpenMVPluginHistogram : public QWidget
{
    Q_OBJECT

public:

    explicit OpenMVPluginHistogram(QWidget *parent = Q_NULLPTR);
    ~OpenMVPluginHistogram();
    void writeColorMap();

public slots:

    void colorSpaceChanged(int colorSpace);
    void pixmapUpdate(const QPixmap &data);
    void captureColorMap(const QPixmap &data, int colorNumber);
    void clearColorMap(const QPixmap &data, int loc);
    void loadColorMap();

signals:
    void updateColorsOnMenu(short cmap[][6]);
    void statusUpdate(char *msg);
    bool script_checkIfRunning();
    void stopClicked();
    void startClicked();

protected:

    bool eventFilter(QObject *watched, QEvent *event);

private:

    void updatePlot(QCPGraph *graph, int channel);

    int m_colorSpace;
    QPixmap m_pixmap;

    int m_mean;
    int m_median;
    int m_mode;
    int m_standardDeviation;
    int m_min;
    int m_max;
    int m_lowerQuartile;
    int m_upperQuartile;

    QCPGraph *m_channel0;
    QCPGraph *m_channel1;
    QCPGraph *m_channel2;
    // DGP
    short m_colormap[8][6];
    bool m_working = false;

    Ui::OpenMVPluginHistogram *m_ui;
};

#endif // OPENMVPLUGINHISTOGRAM_H
