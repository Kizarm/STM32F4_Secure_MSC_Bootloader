##########################################################################
# User configuration and firmware specific object files	
##########################################################################
TARGET    = stm32f4

BLD       = ./obj/
ROOT_PATH = ../..
OUTFILE   = card

VPATH = .

OBJM = dscbld.o

CLEANER += dscbld.c ./bld/*.o ./bld/dscbld
include $(ROOT_PATH)/lib/$(TARGET)/makefile.inc
OBJS  = $(OBJM)

# C / C++ files
OBJS  += main.o
OBJS  += usbuser.o

OBJS+= usbclass.o
OBJS+= usbdevice.o
OBJS+= usbhw.o
OBJS+= usbinterface.o
OBJS+= usbendpoint.o

OBJS+= storagemmc.o
OBJS+= spisimple.o

OBJS+= msclass.o
OBJS+= gpio.o

# COMMON
include $(ROOT_PATH)/makefile.inc

dscbld.c: usbuser.cpp
	cd ./bld && make -f Makefile all
	./bld/dscbld
