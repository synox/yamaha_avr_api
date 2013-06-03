TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp

QMAKE_CXXFLAGS += -std=c++0x
QMAKE_CXXFLAGS = -std=c++11 -stdlib=libc++ -mmacosx-version-min=10.7
QMAKE_LFLAGS = -std=c++11 -stdlib=libc++ -mmacosx-version-min=10.7

INCLUDEPATH += /Applications/Tools/boost/boost_1_53_0/boost/

unix: CONFIG += link_pkgconfig
unix: PKGCONFIG +=

OTHER_FILES += \
    README.md
