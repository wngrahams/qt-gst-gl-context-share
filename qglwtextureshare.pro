TEMPLATE = app
TARGET = qglwtextureshare
QT += opengl gui

# Add console to the CONFIG to see debug messages printed in 
# the console on Windows
# CONFIG += console
DESTDIR = ./debug
DEFINES += UNICODE QT_THREAD_SUPPORT QT_CORE_LIB QT_GUI_LIB
CONFIG += link_pkgconfig
PKGCONFIG=gstreamer-1.0 gstreamer-video-1.0 gstreamer-gl-1.0 #wayland-client

win32 {
DEFINES += WIN32
INCLUDEPATH += \
    C:/gstreamer/include \
    C:/gstreamer/include/libxml2 \
    C:/gstreamer/include/glib-2.0 \
    C:/gstreamer/lib/glib-2.0/include \
    C:/gstreamer/include/gstreamer-1.0
LIBS += -L"C:/gstreamer/lib" \
    -L"C:/gstreamer/bin" \
    -lgstreamer-1.0 \
    -lgstgl-1.0 \
    -lgstvideo-1.0 \
    -lglib-2.0 \
    -lgmodule-2.0 \
    -lgobject-2.0 \
    -lgthread-2.0 \
    -lgstvideo-1.0 \
    -lopengl32 \
    -lglu32
}
unix:!mac {
    #PKGCONFIG += wayland-client
    DEFINES += UNIX
    #INCLUDEPATH += \
    #    /opt/fsl-imx-xwayland/5.4-zeus/sysroots/aarch64-poky-linux/usr/include/QtGui/5.15.0/QtGui \
    #    /opt/fsl-imx-xwayland/5.4-zeus/sysroots/aarch64-poky-linux/usr/include/QtEglSupport/5.15.0
    LIBS += \
        -lgstvideo-1.0 \
        -lgstgl-1.0 #\
        #-lGLU \
        #-lGL
    QT += x11extras gui 
}
mac {
    DEFINES += MACOSX
    INCLUDEPATH += /opt/local/include/ \
        /opt/local/include/gstreamer-1.0/ \
        /opt/local/include/glib-2.0/ \
        /opt/local/lib/glib-2.0/include \
        /opt/local/include/libxml2
    LIBS += -L/opt/local/lib \
        -lgstreamer-1.0 \
        -lgstapp-1.0 \
        -lgstvideo-1.0 \
        -lglib-2.0 \
        -lgobject-2.0 #\
        #-lcxcore \
        #-lcvaux \
        #-lcv
    OBJECTIVE_SOURCES +=  cocoa_utils.mm
    LIBS += -framework AppKit
}
DEPENDPATH += .

# Header files
HEADERS += gstthread.h \
    pipeline.h \
    qglrenderer.h \
    AsyncQueue.h \
    gl-compat-defines.h \
    geometryengine.h

# Source files
SOURCES += gstthread.cpp \
    main.cpp \
    pipeline.cpp \
    qglrenderer.cpp \
    geometryengine.cpp

RESOURCES += \
    shaders.qrc

DEPENDPATH += .
MOC_DIR += ./GeneratedFiles/debug
OBJECTS_DIR += debug
UI_DIR += ./GeneratedFiles
RCC_DIR += ./GeneratedFiles

