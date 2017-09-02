##########################################################################
# GNU GCC compiler flags
##########################################################################
#INCLUDE_PATHS += .
INCLUDE_PATHS += $(ROOT_PATH)/inc
INCLUDE_PATHS += $(ROOT_PATH)/usb
INCLUDE_PATHS += $(ROOT_PATH)/usb/class
INCLUDE_PATHS += $(ROOT_PATH)/lib/cmsis/inc
INCLUDE_PATHS += $(ROOT_PATH)/lib/$(TARGET)/inc
#INCLUDE_PATHS += $(ROOT_PATH)/lib/$(TARGET)/usb/inc

INCLUDES = $(addprefix -I,$(INCLUDE_PATHS))
##########################################################################
# GNU GCC compiler prefix and location
##########################################################################

AS = $(CROSS_COMPILE)gcc
CC = $(CROSS_COMPILE)gcc
CXX= $(CROSS_COMPILE)g++
#LD = $(CROSS_COMPILE)ld
LD = $(CROSS_COMPILE)g++
SIZE = $(CROSS_COMPILE)size
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump

##########################################################################
# Startup files
##########################################################################

LD_PATH  += $(ROOT_PATH)/lib/$(TARGET)

##########################################################################
# Compiler settings, parameters and flags
##########################################################################

CFLAGS  = -c -Os $(INCLUDES) -Wall -g
CFLAGS += $(MFLAGS)
CFLAGS += -ffunction-sections -fdata-sections -fmessage-length=0
CFLAGS += -Wa,-adhlns=$(@:%.o=%.lst)

ASFLAGS = -c -Os $(INCLUDES) -Wall
ASFLAGS+= $(MFLAGS) -D__ASSEMBLY__ -x assembler-with-cpp
#ASFLAGS+= -ffunction-sections -fdata-sections -fmessage-length=0

#LDFLAGS = -nostartfiles
#LDFLAGS+= -nodefaultlibs -nostdlib
#LDFLAGS+= -specs=nano.specs
#LDFLAGS+= --gc-sections -Map=$(OUTFILE).map --cref
LDFLAGS+= $(MFLAGS)
LDFLAGS+= -Wl,--gc-sections,-Map=$(OUTFILE).map,--cref
LDLIBS += $(addprefix -L,$(LD_PATH)) $(LD_SCRIPT)
#LDLIBS += -lm

OCFLAGS = --strip-unneeded

CXXFLAGS = $(CFLAGS)
CXXFLAGS+= -fno-rtti -fno-exceptions
CXXFLAGS+= -fno-use-cxa-atexit

BOBJS = $(addprefix $(BLD),$(OBJS))

all: $(BLD) $(OUTFILE).elf

$(BLD):
	mkdir $(BLD)

$(BLD)%.o : %.c
	$(CC) $(CFLAGS) -std=c99 -o $@ $<

$(BLD)%.o : %.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<
	
$(BLD)%.o : %.s
	$(AS) $(ASFLAGS) -o $@ $<

$(OUTFILE).elf: $(BOBJS)
	$(CXX) $(LDFLAGS) -o $(OUTFILE).elf $(BOBJS) $(LDLIBS)
	-@echo ""
	$(SIZE) $(OUTFILE).elf
	-@echo ""
	$(OBJDUMP) -h -S $(OUTFILE).elf > $(OUTFILE).lst
	$(OBJCOPY) $(OCFLAGS) -O binary $(OUTFILE).elf $(OUTFILE).bin
#	$(OBJCOPY) $(OCFLAGS) -O ihex   $(OUTFILE).elf $(OUTFILE).hex
#	-@echo ""

clean:
	rm -f $(OUTFILE).elf $(OUTFILE).bin $(OUTFILE).hex
	rm -rf $(BLD)
	rm -f $(OUTFILE).map  *.lst
	rm -f $(CLEANER)
#	rm -f $(addsuffix /*~,$(VPATH))
#	rm -f $(addsuffix /*~,$(INCLUDE_PATHS))
