##########################################################################
# User configuration and firmware specific object files	
##########################################################################
#TARGET    = unix
 TARGET    = stm32f4

BLD       = ./obj/
ROOT_PATH = ../..
OUTFILE   = boot

VPATH = .

OBJM = dscbld.o

CLEANER += dscbld.c ./bld/*.o ./bld/dscbld ./fil/FIRMWARE.BIN
include $(ROOT_PATH)/lib/$(TARGET)/boot.inc
OBJS  = $(OBJM)

# C / C++ files
OBJS  += main.o
OBJS  += usbuser.o

OBJS+= usbclass.o
OBJS+= usbdevice.o
OBJS+= usbhw.o
OBJS+= usbinterface.o
OBJS+= usbendpoint.o

OBJS+= gpio.o
OBJS+= fat12emul.o
OBJS+= imagewriter.o
OBJS+= keys.o
OBJS+= tea.o
ifeq ($(TARGET),unix)
OBJS+= unictxt.o
else
OBJS+= jump.o
OBJS+= context.o
OBJS+= checks.o
endif
OBJS+= msclass.o

# COMMON
include $(ROOT_PATH)/makefile.inc

dscbld.c: usbuser.cpp
	cd ./bld && make -f Makefile all
	./bld/dscbld
