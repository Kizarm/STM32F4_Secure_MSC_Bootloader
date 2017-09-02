CPU  ?= cortex-m4
CROSS = arm-none-eabi-

FLAGS = -Wall -c -g -mthumb -mcpu=$(CPU) -Os
FLAGS+= -ffunction-sections -fdata-sections -fmessage-length=0
FLAGS+= -I../inc -I../../cmsis/inc -I../../../inc
AS    = $(CROSS)as -mthumb
CC    = $(CROSS)gcc -std=c99
CXX   = $(CROSS)g++
AR    = $(CROSS)ar
ROOT_PATH = ../../..
TARGET    = stm32f4

OBJS += adcdma.o
OBJS += audio.o
OBJS += cdclass.o
OBJS += gpio.o
OBJS += startup.o
OBJS += system.o
OBJS += usart.o
# USB
include ../usb/makefile.inc
FLAGS += -I../usb/inc
#FLAGS += $(addprefix -I,$(INCLUDE_PATHS))
CC    += $(FLAGS)
CXX   += $(FLAGS) -fno-exceptions -fno-rtti


DSTLIB = libperiph.a

%.o: %.S
	$(AS) $< -o $@
%.o: %.c
	$(CC) $< -o $@
%.o: %.cpp
	$(CXX) $< -o $@

$(DSTLIB): $(OBJS)
	$(AR) -rc $(DSTLIB)  $(OBJS)
	$(AR) -s  $(DSTLIB)
	mv $(DSTLIB) ../



.PHONY: all clean

all: $(BLD)

clean:
	-rm -f *.o *.a
