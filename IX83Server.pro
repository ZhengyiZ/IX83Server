QT = core
QT += network

CONFIG += c++17 cmdline

LIBS += -luser32

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

HEADERS += \
    client.h \
    olympussdk.h \
    olympusdaemonprocess.h \
    olympusportmanager.h \
    server.h

SOURCES += \
    client.cpp \
    main.cpp \
    olympusdaemonprocess.cpp \
    olympusportmanager.cpp \
    server.cpp

RC_ICONS = res/IX83Server.ico

# Copy lib files to build directory
LibFile = $$PWD/lib/*.*
CONFIG(release, debug | release) {
    OutLibFile = $${OUT_PWD}/release/*.*
}else {
    OutLibFile = $${OUT_PWD}/debug/*.*
}
LibFile = $$replace(LibFile, /, \\)
OutLibFile = $$replace(OutLibFile, /, \\)
QMAKE_PRE_LINK += copy $$LibFile $$OutLibFile

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# Copy exe & lib files to specified directory in release mode only
#CONFIG(release, debug | release) {
#    ExeFile = $${OUT_PWD}/release/IX83Server.exe
#    ExeFile = $$replace(ExeFile, /, \\)
#    QMAKE_POST_LINK = copy $$ExeFile D:\\QT\\IX83Server.exe && copy $$LibFile D:\\QT\\*.*
#}
