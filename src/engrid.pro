TEMPLATE = app
LANGUAGE = C++
TARGET = engrid

# install
target.path = /usr/bin

# target.path = $$PREFIX/bin
INSTALLS += target

# CONFIG += qt release thread
# CONFIG += qt debug thread
CONFIG += qt \
    debug_and_release \
    thread

# DEFINES += QT_NO_DEBUG
# DEFINES += QT_DEBUG
# QMAKE_CXXFLAGS += -DAPP_VERSION=\\\"`date +'\"%a_%b_%d,_%Y\"'`\\\"
# ###############################
# VERSION INFO
# get "git revision number"
ENGRID_VERSION = \\\"1.1-pre-release\\\"
win32:GIT_DESCRIBE = \\\"\\\"
else:GIT_DESCRIBE = \\\"$$system(git describe)\\\"
message(GIT_DESCRIBE : $${GIT_DESCRIBE} )
message(ENGRID_VERSION : $${ENGRID_VERSION})
message(Qt version : $$[QT_VERSION])
QMAKE_CXXFLAGS += -DENGRID_VERSION=$${ENGRID_VERSION} \
    -DGIT_DESCRIBE=$${GIT_DESCRIBE}
message(QMAKE_CXXFLAGS : $${QMAKE_CXXFLAGS} )

# ###############################
# to get rid of deprecated header warnings caused by including QVTKwidget.h
# DEFINES += VTK_EXCLUDE_STRSTREAM_HEADERS
# DEFINES += VTK_LEGACY_REMOVE
QMAKE_CXXFLAGS += -Wall

# QMAKE_CXXFLAGS += -O3
# for profiling with gprof
# QMAKE_CXXFLAGS += -pg
# QMAKE_LFLAGS += -pg
QT += xml \
    network \
    opengl

# #######################
# VTK
INCLUDEPATH += $(VTKINCDIR)
LIBS += -L$(VTKLIBDIR)

# #######################
# #######################
# NETGEN
INCLUDEPATH += ./netgen_svn/netgen-mesher/netgen/nglib
INCLUDEPATH += ./netgen_svn/netgen-mesher/netgen/libsrc/general

# #######################
include(engrid-standard.pri)
