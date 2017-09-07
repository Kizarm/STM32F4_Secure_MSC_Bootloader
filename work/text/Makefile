##########################################################################
# User configuration and firmware specific object files	
##########################################################################
#TARGET  ?= unix
 TARGET  ?= stm32f4
BLD       = ./obj/
ROOT_PATH = ../..
OUTFILE   = gsm
ifeq ($(TARGET),unix)
EXLIB     = gsm/libgsmu.a
else
EXLIB     = gsm/libgsma.a
endif

VPATH = . $(ROOT_PATH)/lib/src

include $(ROOT_PATH)/lib/$(TARGET)/user.inc
OBJS  = $(OBJM)

# C++ files
OBJS  += main.o
OBJS  += image.o
OBJS  += gpio.o
OBJS  += audio.o

INCLUDE_PATHS += gsm/inc
LDLIBS += -Lgsm
ifeq ($(TARGET),unix)
LDLIBS += -lgsmu
else
LDLIBS += -lgsma
endif
# COMMON
include $(ROOT_PATH)/makefile.inc

$(EXLIB):
	cd gsm/src && make -f Makefile TARGET=$(TARGET)
