TARGET ?= arm

FLAGS = -c -Os
ifeq ($(TARGET), unix)
else
CPU  ?= cortex-m4
CROSS = arm-none-eabi-
FLAGS+= -mthumb -mcpu=$(CPU)
FLAGS+= -ffunction-sections -fdata-sections
endif
FLAGS+= -I../inc
FLAGS+= -DNeedFunctionPrototypes=1 -DSASR -DWAV49 -DNDEBUG=1
AS    = $(CROSS)as -mthumb
CC    = $(CROSS)gcc
CC   += $(FLAGS)
CXX   = $(CROSS)g++
CXX  += $(FLAGS) -fno-exceptions -fno-rtti
AR    = $(CROSS)ar

CFILES = $(wildcard *.c)
CXFILES= $(wildcard *.cpp)

OBJS   = $(CFILES:%.c=%.o)
OBJS  += $(CXFILES:%.cpp=%.o)

ifeq ($(TARGET), unix)
DSTLIB = ../libgsmu.a
BLD = ./bldu/
else
DSTLIB = ../libgsma.a
BLD = ./blda/
endif
BOBJS = $(addprefix $(BLD),$(OBJS))

all: $(BLD) $(DSTLIB)

$(BLD)%.o: %.S
	$(AS) $< -o $@
$(BLD)%.o: %.c
	$(CC) $< -o $@
$(BLD)%.o: %.cpp
	$(CXX) $< -o $@

$(DSTLIB): $(BOBJS)
	$(AR) rcs $(DSTLIB) $(BOBJS)
$(BLD):
	mkdir $(BLD)

clean:
	-rm -rf $(BLD)
	-rm -f *.a

.PHONY: all clean
