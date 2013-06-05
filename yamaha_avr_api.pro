TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp pugixml.cpp

QMAKE_CXXFLAGS += -std=c++0x
QMAKE_CXXFLAGS = -std=c++11 -stdlib=libc++ -mmacosx-version-min=10.7
QMAKE_LFLAGS = -std=c++11 -stdlib=libc++ -mmacosx-version-min=10.7


# include ibcurl:
INCLUDEPATH += /usr/local/include /usr/local/opt/openssl/include
LIBS += -lidn -lcurl -lssl -lncurses


# boost libs
#INCLUDEPATH += /Applications/Tools/boost_bin/include
#LIBS += -L/Applications/Tools/boost_bin/lib
#LIBS += -lboost_system -lboost_regex -lboost_thread

HEADERS+=pugiconfig.hpp pugixml.hpp \
    status.hpp

unix: CONFIG += link_pkgconfig
unix: PKGCONFIG +=

OTHER_FILES += \
    README.md
