##########################################################################
# User configuration and firmware specific object files	
##########################################################################
TARGET    = unix
#TARGET    = stm32f4

BLD       = ./obj/
ROOT_PATH = ../..
OUTFILE   = disc

VPATH = .
CLEANER += ./bld/*.o ./bld/dscbld dscbld.c disk.img

include $(ROOT_PATH)/lib/$(TARGET)/makefile.inc
OBJS  = $(OBJM)

# C++ files
OBJS  += dscbld.o
OBJS  += main.o
OBJS  += usbuser.o

OBJS+= usbclass.o
OBJS+= usbdevice.o
OBJS+= usbhw.o
OBJS+= usbinterface.o
OBJS+= usbendpoint.o

OBJS+= storagememory.o
OBJS+= image.o

OBJS+= msclass.o
#OBJS+= cdclass.o

# COMMON
include $(ROOT_PATH)/makefile.inc

dscbld.c: usbuser.cpp
	cd ./bld && make -f Makefile all
	./bld/dscbld
image.s: disk.img
./mnt:
	mkdir ./mnt
# Obraz disku si pripravime standardni unixovou metodou. Minimalni velikost je 66x512 byte
disk.img: ./mnt
	dd if=/dev/zero of=disk.img bs=512 count=132
	sudo losetup /dev/loop0 ./disk.img
	sudo mkfs.fat -v -F 12 -f 1 -r 16 -S 512 -s 2 -n BOOTLOADER /dev/loop0
	sudo mount /dev/loop0 ./mnt
	sudo cp ./fil/* ./mnt
	sudo umount ./mnt
	sudo losetup -d /dev/loop0
