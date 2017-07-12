#include "openmvpluginfb.h"
#include <time.h>

OpenMVPluginFB::OpenMVPluginFB(QWidget *parent) : QGraphicsView(parent)
{
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setFrameStyle(QFrame::NoFrame);
    setMinimumWidth(160);
    setMinimumHeight(120);
    setBackgroundBrush(QColor(30, 30, 39));
    setScene(new QGraphicsScene(this));

    QGraphicsTextItem *item = new QGraphicsTextItem;
    item->setHtml(tr("<html><body style=\"color:#909090;font-size:14px\">"
    "<div align=\"center\">"
    "<div style=\"font-size:20px\">No Image</div>"
    "</div>"
    "</body></html>"));
    scene()->addItem(item);

    m_enableFitInView = false;
    m_pixmap = Q_NULLPTR;
    m_enableSaveTemplate = false;
    m_enableSaveDescriptor = false;
    m_unlocked = false;
    m_origin = QPoint();

    m_band = new QRubberBand(QRubberBand::Rectangle, this);
    m_band->setGeometry(QRect());
    m_band->hide();
}

bool OpenMVPluginFB::pixmapValid() const
{
    return m_pixmap;
}

QPixmap OpenMVPluginFB::pixmap() const
{
    return m_pixmap ? m_pixmap->pixmap() : QPixmap();
}

void OpenMVPluginFB::enableFitInView(bool enable)
{
    m_enableFitInView = enable;

    if(m_pixmap)
    {
        myFitInView();
    }

    if(m_band->isVisible())
    {
        m_unlocked = false;
        m_band->setGeometry(QRect());
        m_band->hide();

        // Broadcast the new pixmap
        emit pixmapUpdate(getPixmap());
    }
}

void OpenMVPluginFB::updateColorsOnMenu(short cmap[][6])
{
    char msg[200];
    int colorNumber;

    for (colorNumber=0; colorNumber < 8; colorNumber++) {
        m_ColormapCopy[colorNumber][0] = cmap[colorNumber][0];
        m_ColormapCopy[colorNumber][1] = cmap[colorNumber][1];
        m_ColormapCopy[colorNumber][2] = cmap[colorNumber][2];
        m_ColormapCopy[colorNumber][3] = cmap[colorNumber][3];
        m_ColormapCopy[colorNumber][4] = cmap[colorNumber][4];
        m_ColormapCopy[colorNumber][5] = cmap[colorNumber][5];
    }
}

void OpenMVPluginFB::frameBufferData(const QPixmap &data)
{
    delete scene();
    setScene(new QGraphicsScene(this));

    m_pixmap = scene()->addPixmap(data);

    myFitInView();

    // Broadcast the new pixmap
    emit pixmapUpdate(getPixmap());
}

void OpenMVPluginFB::mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton)
    {
        m_unlocked = m_pixmap && (m_pixmap == itemAt(event->pos()));
        m_origin = event->pos();
        m_band->setGeometry(QRect());
        m_band->hide();

        // Broadcast the new pixmap
        emit pixmapUpdate(getPixmap());
    }

    QGraphicsView::mousePressEvent(event);
}

void OpenMVPluginFB::mouseMoveEvent(QMouseEvent *event)
{
    if(m_unlocked)
    {
        m_band->setGeometry(QRect(m_origin, event->pos()).normalized().intersected(mapFromScene(sceneRect()).boundingRect()));
        m_band->show();

        // Broadcast the new pixmap
        emit pixmapUpdate(getPixmap());
    }

    QGraphicsView::mouseMoveEvent(event);
}

void OpenMVPluginFB::mouseReleaseEvent(QMouseEvent *event)
{
    m_unlocked = false;

    QGraphicsView::mouseReleaseEvent(event);
}

void OpenMVPluginFB::contextMenuEvent(QContextMenuEvent *event)
{
    short cmap[8][6] = {
        {0,0,0,0,0,0},
        {1,0,0,0,0,0},
        {2,0,0,0,0,0},
        {3,0,0,0,0,0},
        {4,0,0,0,0,0},
        {5,0,0,0,0,0},
        {6,0,0,0,0,0},
        {7,0,0,0,0,0}
    };
    if(m_pixmap && (m_pixmap == itemAt(event->pos())))
    {
        bool cropped;
        QRect croppedRect;
        char msg[200];
        int colorNumber;
        QPixmap pixmap = getPixmap(true, event->pos(), &cropped, &croppedRect);

        QMenu menu(this);

        QAction *sColorMap1;
        QAction *sColorMap2;
        QAction *sColorMap3;
        QAction *sColorMap4;
        QAction *sColorMap5;
        QAction *sColorMap6;
        QAction *sColorMap7;
        QAction *sColorMap8;
        if ( cropped ) {
            sColorMap1 = menu.addAction(tr("Save To Colormap 1"));
            sColorMap2 = menu.addAction(tr("Save To Colormap 2"));
            sColorMap3 = menu.addAction(tr("Save To Colormap 3"));
            sColorMap4 = menu.addAction(tr("Save To Colormap 4"));
            sColorMap5 = menu.addAction(tr("Save To Colormap 5"));
            sColorMap6 = menu.addAction(tr("Save To Colormap 6"));
            sColorMap7 = menu.addAction(tr("Save To Colormap 7"));
            sColorMap8 = menu.addAction(tr("Save To Colormap 8"));
            menu.addSeparator();
        } else {
            sColorMap1 = sColorMap2 = sColorMap3 = sColorMap4 = sColorMap5 =
                sColorMap6 = sColorMap7 = sColorMap8 =  NULL;
        }
        colorNumber = 0;
        sprintf(msg, "Clear Colormap %d, (%d:%d:%d:%d:%d:%d)", colorNumber+1,
                m_ColormapCopy[colorNumber][0], m_ColormapCopy[colorNumber][1],
                m_ColormapCopy[colorNumber][2], m_ColormapCopy[colorNumber][3],
                m_ColormapCopy[colorNumber][4], m_ColormapCopy[colorNumber][5]);
        QAction *sclearColor1 = menu.addAction(tr(msg));
        colorNumber = 1;
        sprintf(msg, "Clear Colormap %d, (%d:%d:%d:%d:%d:%d)", colorNumber+1,
                m_ColormapCopy[colorNumber][0], m_ColormapCopy[colorNumber][1],
                m_ColormapCopy[colorNumber][2], m_ColormapCopy[colorNumber][3],
                m_ColormapCopy[colorNumber][4], m_ColormapCopy[colorNumber][5]);
        QAction *sclearColor2 = menu.addAction(tr(msg));
        colorNumber = 2;
        sprintf(msg, "Clear Colormap %d, (%d:%d:%d:%d:%d:%d)", colorNumber+1,
                m_ColormapCopy[colorNumber][0], m_ColormapCopy[colorNumber][1],
                m_ColormapCopy[colorNumber][2], m_ColormapCopy[colorNumber][3],
                m_ColormapCopy[colorNumber][4], m_ColormapCopy[colorNumber][5]);
        QAction *sclearColor3 = menu.addAction(tr(msg));
        colorNumber = 3;
        sprintf(msg, "Clear Colormap %d, (%d:%d:%d:%d:%d:%d)", colorNumber+1,
                m_ColormapCopy[colorNumber][0], m_ColormapCopy[colorNumber][1],
                m_ColormapCopy[colorNumber][2], m_ColormapCopy[colorNumber][3],
                m_ColormapCopy[colorNumber][4], m_ColormapCopy[colorNumber][5]);
        QAction *sclearColor4 = menu.addAction(tr(msg));
        colorNumber = 4;
        sprintf(msg, "Clear Colormap %d, (%d:%d:%d:%d:%d:%d)", colorNumber+1,
                m_ColormapCopy[colorNumber][0], m_ColormapCopy[colorNumber][1],
                m_ColormapCopy[colorNumber][2], m_ColormapCopy[colorNumber][3],
                m_ColormapCopy[colorNumber][4], m_ColormapCopy[colorNumber][5]);
        QAction *sclearColor5 = menu.addAction(tr(msg));
        colorNumber = 5;
        sprintf(msg, "Clear Colormap %d, (%d:%d:%d:%d:%d:%d)", colorNumber+1,
                m_ColormapCopy[colorNumber][0], m_ColormapCopy[colorNumber][1],
                m_ColormapCopy[colorNumber][2], m_ColormapCopy[colorNumber][3],
                m_ColormapCopy[colorNumber][4], m_ColormapCopy[colorNumber][5]);
        QAction *sclearColor6 = menu.addAction(tr(msg));
        colorNumber = 6;
        sprintf(msg, "Clear Colormap %d, (%d:%d:%d:%d:%d:%d)", colorNumber+1,
                m_ColormapCopy[colorNumber][0], m_ColormapCopy[colorNumber][1],
                m_ColormapCopy[colorNumber][2], m_ColormapCopy[colorNumber][3],
                m_ColormapCopy[colorNumber][4], m_ColormapCopy[colorNumber][5]);
        QAction *sclearColor7 = menu.addAction(tr(msg));
        colorNumber = 7;
        sprintf(msg, "Clear Colormap %d, (%d:%d:%d:%d:%d:%d)", colorNumber+1,
                m_ColormapCopy[colorNumber][0], m_ColormapCopy[colorNumber][1],
                m_ColormapCopy[colorNumber][2], m_ColormapCopy[colorNumber][3],
                m_ColormapCopy[colorNumber][4], m_ColormapCopy[colorNumber][5]);

        QAction *sclearColor8 = menu.addAction(tr(msg));
        menu.addSeparator();
        QAction *sclearColorAll = menu.addAction(tr("Clear all Colormaps"));
        QAction *sRestoreDefault = menu.addAction(tr("Restore Default Colormaps"));
        menu.addSeparator();

        QAction *sImage = menu.addAction(cropped ? tr("Save Image selection to PC") : tr("Save Image to PC"));
        /*
        menu.addSeparator();
        QAction *sTemplate = menu.addAction(cropped ? tr("Save Template selection to Cam") : tr("Save Template to Cam"));
        sTemplate->setVisible(m_enableSaveTemplate);
        QAction *sDescriptor = menu.addAction(cropped ? tr("Save Descriptor selection to Cam") : tr("Save Descriptor to Cam"));
        sDescriptor->setVisible(m_enableSaveDescriptor);
        */

        QAction *selected = menu.exec(event->globalPos());

        if ( selected != NULL ) {
            if(selected == sColorMap1) {
                emit captureColorMap(pixmap, 0);
            }
            else if(selected == sColorMap2) {
                emit captureColorMap(pixmap, 1);
            }
            else if(selected == sColorMap3) {
                emit captureColorMap(pixmap, 2);
            }
            else if(selected == sColorMap4) {
                emit captureColorMap(pixmap, 3);
            }
            else if(selected == sColorMap5) {
                emit captureColorMap(pixmap, 4);
            }
            else if(selected == sColorMap6) {
                emit captureColorMap(pixmap, 5);
            }
            else if(selected == sColorMap7) {
                emit captureColorMap(pixmap, 6);
            }
            else if(selected == sColorMap8) {
                emit captureColorMap(pixmap, 7);
            }
            else if(selected == sclearColor1) {
                emit clearColorMap(pixmap, 0);
            }
            else if(selected == sclearColor2) {
                emit clearColorMap(pixmap, 1);
            }
            else if(selected == sclearColor3) {
                emit clearColorMap(pixmap, 2);
            }
            else if(selected == sclearColor4) {
                emit clearColorMap(pixmap, 3);
            }
            else if(selected == sclearColor5) {
                emit clearColorMap(pixmap, 4);
            }
            else if(selected == sclearColor6) {
                emit clearColorMap(pixmap, 5);
            }
            else if(selected == sclearColor7) {
                emit clearColorMap(pixmap, 6);
            }
            else if(selected == sclearColor8) {
                emit clearColorMap(pixmap, 7);
            }
            else if(selected == sclearColorAll) {
                emit clearColorMap(pixmap, -1);
            }
            else if(selected == sRestoreDefault) {
                emit clearColorMap(pixmap, -2);
            }

            else if(selected == sImage) {
                emit saveImage(pixmap);
            }
            /*
            else if(selected == sTemplate) {
                emit saveTemplate(croppedRect);
            }
            else if(selected == sDescriptor) {
                emit saveDescriptor(croppedRect);
            }
            */
        }
    }

}
#if(0)
void OpenMVPluginFB::contextMenuEvent_orig(QContextMenuEvent *event)
{
    if(m_pixmap && (m_pixmap == itemAt(event->pos())))
    {
        bool cropped;
        QRect croppedRect;
        QPixmap pixmap = getPixmap(true, event->pos(), &cropped, &croppedRect);

        QMenu menu(this);

        QAction *sImage = menu.addAction(cropped ? tr("Save Image selection to PC") : tr("Save Image to PC"));
        menu.addSeparator();
        QAction *sTemplate = menu.addAction(cropped ? tr("Save Template selection to Cam") : tr("Save Template to Cam"));
        sTemplate->setVisible(m_enableSaveTemplate);
        QAction *sDescriptor = menu.addAction(cropped ? tr("Save Descriptor selection to Cam") : tr("Save Descriptor to Cam"));
        sDescriptor->setVisible(m_enableSaveDescriptor);

        QAction *selected = menu.exec(event->globalPos());

        if(selected == sImage)
        {
            emit saveImage(pixmap);
        }
        else if(selected == sTemplate)
        {
            emit saveTemplate(croppedRect);
        }
        else if(selected == sDescriptor)
        {
            emit saveDescriptor(croppedRect);
        }
    }
}
#endif

void OpenMVPluginFB::resizeEvent(QResizeEvent *event)
{
    if(m_pixmap)
    {
        myFitInView();
    }

    if(m_band->isVisible())
    {
        m_unlocked = false;
        m_band->setGeometry(QRect());
        m_band->hide();

        // Broadcast the new pixmap
        emit pixmapUpdate(getPixmap());
    }

    QGraphicsView::resizeEvent(event);
}

QPixmap OpenMVPluginFB::getPixmap(bool pointValid, const QPoint &point, bool *cropped, QRect *croppedRect)
{
    if(!m_pixmap)
    {
        return QPixmap();
    }

    bool crop = m_band->geometry().isValid() && ((!pointValid) || m_band->geometry().contains(point));

    if(cropped)
    {
        *cropped = crop;
    }

    QRect rect = m_pixmap->mapFromScene(mapToScene(m_band->geometry())).boundingRect().toRect();

    if(croppedRect)
    {
        *croppedRect = crop ? rect : m_pixmap->pixmap().rect();
    }

    if(crop)
    {
        return m_pixmap->pixmap().copy(rect);
    }

    return m_pixmap->pixmap();
}

void OpenMVPluginFB::myFitInView()
{
    qreal scale = qMin(width() / sceneRect().width(), height() / sceneRect().height());
    QTransform matrix(1, 0, 0, 0, 1, 0, 0, 0, 1);

    if(m_enableFitInView)
    {
        matrix.scale(scale, scale);
    }

    setTransform(matrix);
}
