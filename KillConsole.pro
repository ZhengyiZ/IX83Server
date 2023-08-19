QT = core
QT += network

LIBS += -lws2_32

CONFIG += c++17 cmdline

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        killconsole.cpp

RC_ICONS = res/KillConsole.ico

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# Copy exe & lib files to specified directory in release mode only
#CONFIG(release, debug | release) {
#    ExeFile = $${OUT_PWD}/release/KillConsole.exe
#    ExeFile = $$replace(ExeFile, /, \\)
#    QMAKE_POST_LINK = copy $$ExeFile D:\\QT\\KillConsole.exe
#}
