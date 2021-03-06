TEMPLATE = app

QT -= gui
CONFIG -= qt
DEFINES -= UNICODE QT_LARGEFILE_SUPPORT

CONFIG += console

TARGET = NewPanOptTester

LIBS += -pthread

CONFIG(release, debug|release) {
    message("NewPanOptTester: Release mode!")
    DEFINES += NDEBUG
    QMAKE_CXXFLAGS_RELEASE += -O2 -std=c++11 -mavx
    QMAKE_LFLAGS = -static-libgcc -static-libstdc++
    POST_TARGETDEPS += ../build-NewPanOpt/release/libNewPanOpt.a
    LIBS += -L../build-NewPanOpt/release/ -lNewPanOpt
    OBJECTS_DIR = .o_release

}
CONFIG(debug, debug|release) {
    message("NewPanOptTester: Debug mode!")
    QMAKE_CXXFLAGS_DEBUG += -g -std=c++11
    QMAKE_LFLAGS = -static-libgcc -static-libstdc++
    POST_TARGETDEPS += ../build-NewPanOpt/debug/libNewPanOpt.a
    LIBS += -L../build-NewPanOpt/debug/ -lNewPanOpt
    OBJECTS_DIR = .o_debug

}

#Includes
INCLUDEPATH += . \
               include/ \
               test/

#Input
HEADERS += test/framework/tester.h \
           test/framework/unittest.h \
           test/constrainttest.h \
           test/indexlisttest.h \
           test/manualmodelbuildertest.h \
           test/modeltest.h \
           test/mpsproblemtest.h \
           test/testmacros.h \
           test/variabletest.h \
           test/vectortest.h \
           test/timertest.h \
           test/linalgparameterhandlertest.h \
           test/simplexparameterhandlertest.h \
    test/outputhandlertest.h \ #\
    test/coretest.h \
    test/sparsevectortest.h \
    test/framework/report.h \
    test/framework/reportgenerator.h \
    test/framework/htmlreportgenerator.h \
    test/framework/latexreportgenerator.h \
    test/framework/mysqlreportgenerator.h \
    test/framework/sqlitereportgenerator.h \
    test/framework/xmlreportgenerator.h \
    test/framework/sqlreportgenerator.h \
    test/framework/filereportgenerator.h
   # test/dualratiotesttest.h

#Sources
SOURCES += test/framework/tester.cpp \
           test/test.cpp \
           test/indexlisttest.cpp \
           test/modeltest.cpp \
           test/mpsproblemtest.cpp \
           test/timertest.cpp \
           test/linalgparameterhandlertest.cpp \
           test/simplexparameterhandlertest.cpp \
    test/outputhandlertest.cpp \
    test/coretest.cpp \
    test/sparsevectortest.cpp \
    test/framework/report.cpp \
    test/framework/htmlreportgenerator.cpp \
    test/framework/latexreportgenerator.cpp \
    test/framework/mysqlreportgenerator.cpp \
    test/framework/sqlitereportgenerator.cpp \
    test/framework/sqlreportgenerator.cpp \
    test/framework/xmlreportgenerator.cpp \
    test/framework/filereportgenerator.cpp
  #  test/dualratiotesttest.cpp

OBJECTS_DIR = .o
