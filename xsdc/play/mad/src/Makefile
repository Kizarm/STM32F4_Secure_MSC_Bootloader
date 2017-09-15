# Jednoduchy priklad makefile pro vytvoreni staticke knihovny
# a linkovani programu pomoci ni
#TARGET ?= unix
 TARGET ?= stm32f4

DEFINE = -DHAVE_CONFIG_H
ifeq ($(TARGET),stm32f4)
DEFINE+= -DFPM_ARM
MFLAGS = -mthumb -mcpu=cortex-m4
CROSS  = arm-none-eabi-
else
DEFINE+= -DFPM_DEFAULT
endif
CC     = $(CROSS)gcc
AR     = $(CROSS)ar

CFLAGS = $(MFLAGS)
CFLAGS+= $(DEFINE)
CFLAGS+= -I. -I../inc -Wall -Os
CFLAGS+= -ffunction-sections -fdata-sections
ifeq ($(TARGET),stm32f4)
MY_LIB = ../libmada.a
BLD = ./obja/
else
MY_LIB = ../libmadu.a
BLD = ./obju/
endif
BOBJS = $(addprefix $(BLD),$(OBJS))

all: $(BLD) $(MY_LIB)

OBJS = bit.o decoder.o fixed.o frame.o huffman.o layer12.o
OBJS+= layer3.o stream.o synth.o timer.o

$(BLD)%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

$(MY_LIB): $(BOBJS)
	$(AR) rcs  $(MY_LIB) $(BOBJS)
$(BLD):
	mkdir $(BLD)
clean:
	rm -f $(MY_LIB)
	rm -rf $(BLD)
