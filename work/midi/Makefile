##########################################################################
# User configuration and firmware specific object files	
##########################################################################
ifeq ($(OS),Windows_NT)
        MOV = move .\melody\$(MIDI).c .
        GEN = gen.exe
else
        MOV = mv ./melody/$(MIDI).c .
        GEN = ./gen
endif
##########################################################################
 TARGET    = stm32f4
#TARGET    = unix

BLD       = ./obj/
ROOT_PATH = ../..
OUTFILE   = playmidi

VPATH = .

#MIDI ?= Elise
 MIDI ?= podraz

include $(ROOT_PATH)/lib/$(TARGET)/user.inc
OBJS  = $(OBJM)
# aditional clean files
CLEANER += miditone.c $(MIDI).c ./hlp/gen ./hlp/miditones
# C++ files
#OBJS  += main.o
OBJS  += gpio.o
OBJS  += midiplayer.o
OBJS  += tone.o
OBJS  += audio.o
OBJS  += efect.o
#OBJS  += audiopwm.o
# C files
OBJS  += exti0.o

OBJS  += miditone.o
OBJS  += $(MIDI).o

  MAXGENS = 16
  GENCMD = hlp/miditones -s2 -t$(MAXGENS) ./melody/$(MIDI) && $(MOV)
  OBJS  += main.o

# COMMON
include $(ROOT_PATH)/makefile.inc

$(MIDI).c: hlp/miditones
	$(GENCMD)
hlp/miditones: hlp/miditonesV1.6.c
	gcc -o hlp/miditones hlp/miditonesV1.6.c

miditone.c: hlp/gen
	cd hlp && $(GEN)
hlp/gen: hlp/gen.c
	g++ -I$(ROOT_PATH)/lib/$(TARGET)/inc -o hlp/gen hlp/gen.c -lm

.PHONY: all clean
