# SailJami - Jami client for Sailfish OS
# Copyright (C) 2026 SailJami contributors
# SPDX-License-Identifier: GPL-3.0-or-later

TARGET = sailjami

CONFIG += sailfishapp c++17

# Prefix where libjami (jami-daemon built with BUILD_SHARED_LIBS=ON) is installed.
# scripts/build-libjami.sh installs it to ../daemon/install-<qt arch> by default
# (i386, arm or arm64).
isEmpty(JAMI_PREFIX): JAMI_PREFIX = $$PWD/../daemon/install-$$QT_ARCH
!exists($$JAMI_PREFIX/include/jami/jami.h): error("libjami not found in $$JAMI_PREFIX, run scripts/build-libjami.sh first")

INCLUDEPATH += $$JAMI_PREFIX/include
LIBS += -L$$JAMI_PREFIX/lib -ljami

# Private library location, allowed for Sailfish applications.
QMAKE_RPATHDIR += /usr/share/$${TARGET}/lib

jamilib.files = $$files($$JAMI_PREFIX/lib/libjami.so*)
jamilib.path = /usr/share/$${TARGET}/lib
jamilib.CONFIG += no_check_exist
INSTALLS += jamilib

SOURCES += \
    src/main.cpp \
    src/jamibackend.cpp \
    src/conversationlistmodel.cpp \
    src/messagemodel.cpp

HEADERS += \
    src/jamibackend.h \
    src/conversationlistmodel.h \
    src/messagemodel.h

DISTFILES += \
    qml/sailjami.qml \
    qml/cover/CoverPage.qml \
    qml/components/Avatar.qml \
    qml/pages/*.qml \
    rpm/sailjami.spec \
    sailjami.desktop

SAILFISHAPP_ICONS = 86x86 108x108 128x128 172x172
