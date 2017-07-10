include(../../qtcreatorplugin.pri)
QT += concurrent gui-private network printsupport serialport
HEADERS += openmvplugin.h \
           openmvpluginserialport.h \
           openmvpluginio.h \
           openmvpluginfb.h \
           openmvterminal.h \
           histogram/openmvpluginhistogram.h \
           tools/thresholdeditor.h \
           tools/keypointseditor.h \
           tools/tag16h5.h \
           tools/tag25h7.h \
           tools/tag25h9.h \
           tools/tag36h10.h \
           tools/tag36h11.h \
           tools/tag36artoolkit.h \
           qcustomplot/qcustomplot.h
SOURCES += openmvplugin.cpp \
           openmvpluginserialport.cpp \
           openmvpluginio.cpp \
           openmvpluginfb.cpp  \
           openmvterminal.cpp \
           histogram/openmvpluginhistogram.cpp \
           histogram/rgb2rgb_tab.c \
           histogram/lab_tab.c \
           histogram/yuv_tab.c  \
           tools/thresholdeditor.cpp \
           tools/keypointseditor.cpp \
           tools/tag16h5.c \
           tools/tag25h7.c \
           tools/tag25h9.c \
           tools/tag36h10.c \
           tools/tag36h11.c \
           tools/tag36artoolkit.c \
           qcustomplot/qcustomplot.cpp
FORMS += histogram/openmvpluginhistogram.ui
RESOURCES += openmv.qrc
