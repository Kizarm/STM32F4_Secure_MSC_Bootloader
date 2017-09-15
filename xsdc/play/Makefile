##########################################################################
# User configuration and firmware specific object files	
##########################################################################
#TARGET  ?= unix
 TARGET  ?= stm32f4
BLD       = ./obj/
ROOT_PATH = ../..
OUTFILE   = play
ifeq ($(TARGET),unix)
EXLIB     = mad/libmadu.a
else
EXLIB     = mad/libmada.a
endif

VPATH = . $(ROOT_PATH)/lib/src

include $(ROOT_PATH)/lib/$(TARGET)/makefile.inc
OBJS  = $(OBJM)

# C++ files
OBJS += main.o
OBJS += gpio.o
OBJS += audiodma.o
OBJS += minimad.o

OBJS += fat32ro.o

ifeq ($(TARGET),unix)
OBJS += diskfile.o
else
OBJS += diskcard.o
OBJS += spisimple.o
endif

INCLUDE_PATHS += mad/inc
LDLIBS += -Lmad
ifeq ($(TARGET),unix)
LDLIBS += -lmadu
else
LDLIBS += -lmada
endif
# COMMON
include $(ROOT_PATH)/makefile.inc

$(EXLIB):
	cd mad/src && make -f Makefile TARGET=$(TARGET)
