QT += core gui widgets
CONFIG += c++17

INCLUDEPATH += /usr/include/python3.10
INCLUDEPATH += ../matplotlibcpp

# Link against Python libraries
LIBS += -lpython3.10

HEADERS += crosscorrelationapp.h
SOURCES += crosscorrelationapp.cpp main.cpp

