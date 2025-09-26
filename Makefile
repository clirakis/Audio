##################################################################
#
#	Makefile for Audio using gcc on Linux. 
#
#
#	Modified	by	Reason
# 	--------	--	------
#	26-Sep-25       CBL     Original
#
#
######################################################################
# Machine specific stuff
#
#
TARGET = Audio
#
# Compile time resolution.
#
INCLUDE = -I$(DRIVE)/common/utility \
	-I/usr/include/hdf5/serial
LIBS = -lutility -lhdf5_cpp -lhdf5
LIBS += -L$(HDF5LIB) -lconfig++ -lportaudio -lfftw3


# Rules to make the object files depend on the sources.
SRC     = 
SRCCPP  = main.cpp MainModule.cpp Analysis.cpp UserSignals.cpp
SRCS    = $(SRC) $(SRCCPP)

HEADERS = MainModule.hh Analysis.hh UserSignals.hh Version.hh

# When we build all, what do we build?
all:      $(TARGET)

include $(DRIVE)/common/makefiles/makefile.inc
