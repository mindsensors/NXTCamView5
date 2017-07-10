#ifndef THRESHOLDEDITOR_H
#define THRESHOLDEDITOR_H

#include <QtCore>
#include <QtGui>
#include <QtWidgets>

class ThresholdEditor : public QDialog
{
    Q_OBJECT

public:

    explicit ThresholdEditor(const QPixmap &pixmap, QByteArray geometry, QWidget *parent = Q_NULLPTR, Qt::WindowFlags f = Qt::WindowFlags());

public slots:

    void changed();

protected:

    void resizeEvent(QResizeEvent *event);
    void showEvent(QShowEvent *event);

private:

    QImage m_image;
    QGraphicsView *m_raw, *m_bin;
    QComboBox *m_combo;
    QCheckBox *m_invert;
    QWidget *m_paneG, *m_paneLAB;
    QSlider *m_GMin, *m_GMax, *m_LMin, *m_LMax, *m_AMin, *m_AMax, *m_BMin, *m_BMax;
    QLineEdit *m_GOut, *m_LABOut;
    QByteArray m_geometry;
};

#endif // THRESHOLDEDITOR_H
