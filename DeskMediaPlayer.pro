QT       += core gui multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

lessThan(QT_MAJOR_VERSION, 6): win32: QMAKE_CXXFLAGS += -execution-charset:utf-8

# make the application have authority of running on Ubuntu, and I do not know why it works
unix: QMAKE_LFLAGS += -no-pie

# Qt 5.14 VERSION can only contains numbers, no any others type of character please
# please do not put 0 before any version number, because this will cause a warnning on Qt 5.14
win32:  VERSION = 23.01.18.1554                # major.minor.patch.build
else:   VERSION = 21.5.9                    # major.minor.patch

QMAKE_TARGET_COPYRIGHT = Mtr1994 User Co., Ltd

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# 禁用 QDebug 输出调试i信息
#DEFINES += QT_NO_DEBUG_OUTPUT


# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH += $$PWD/Sdk/ffmpeg/include

DESTDIR = ../output

SOURCES += \
    Configure/softconfig.cpp \
    Control/Drawer/widgetdrawer.cpp \
    Control/Message/messagewidget.cpp \
    Dialog/dialogversion.cpp \
    Player/widgetplayer.cpp \
    Public/appsignal.cpp \
    Widget/widgetmediacontrol.cpp \
    Widget/widgetmediapathlist.cpp \
    Widget/widgetslider.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    Common/common.h \
    Configure/softconfig.h \
    Control/Drawer/widgetdrawer.h \
    Control/Message/messagewidget.h \
    Dialog/dialogversion.h \
    Player/widgetplayer.h \
    Public/appsignal.h \
    Public/ffmpeg.h \
    Public/threadsafequeue.h \
    Widget/widgetmediacontrol.h \
    Widget/widgetmediapathlist.h \
    Widget/widgetslider.h \
    mainwindow.h

FORMS += \
    Dialog/dialogversion.ui \
    Widget/widgetmediacontrol.ui \
    Widget/widgetmediapathlist.ui \
    mainwindow.ui

TRANSLATIONS += \
    Translate_zh_CN.ts

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RC_ICONS = ./Resource/icon/icon.ico

RESOURCES += \
    res.qrc

#0x0800代表和系统当前语言一致
RC_LANG = 0x0800

LIBS += -L$$PWD/Sdk/ffmpeg/lib \
-lavformat \
-lavcodec \
-lavdevice \
-lavfilter \
-lavutil \
-lswresample \
-lswscale \

DISTFILES +=
