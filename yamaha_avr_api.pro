TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp

QMAKE_CXXFLAGS += -std=c++0x
QMAKE_CXXFLAGS = -std=c++11 -stdlib=libc++ -mmacosx-version-min=10.7
QMAKE_LFLAGS = -std=c++11 -stdlib=libc++ -mmacosx-version-min=10.7


INCLUDEPATH += /Applications/Tools/boost_bin/include
LIBS += -L/Applications/Tools/boost_bin/lib
LIBS += -lboost_system -lboost_filesystem -lboost_regex


unix: CONFIG += link_pkgconfig
unix: PKGCONFIG +=

OTHER_FILES += \
    README.md
