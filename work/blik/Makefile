##########################################################################
# User configuration and firmware specific object files	
##########################################################################
TARGET    = stm32f4
BLD       = ./obj/
ROOT_PATH = ../..
OUTFILE   = firmware

VPATH = .


include $(ROOT_PATH)/lib/$(TARGET)/user.inc
OBJS  = $(OBJM)

# C++ files
OBJS  += main.o
OBJS  += gpio.o

# EXTRA FILES
#OBJS     += ...
#VPATH         += ./src
#INCLUDE_PATHS += ./inc
#LD_PATH  += ...
#LDLIBS += -lperiph

# COMMON
include $(ROOT_PATH)/makefile.inc
