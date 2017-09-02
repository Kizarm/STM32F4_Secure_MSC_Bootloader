CROSS_COMPILE = 
CPU_TYPE      = 
MFLAGS        = 

VPATH += $(ROOT_PATH)/src
VPATH += $(ROOT_PATH)/usb
VPATH += $(ROOT_PATH)/usb/class
VPATH += $(ROOT_PATH)/lib/$(TARGET)/src

LDLIBS += -lpthread -lasound

#OBJM  =
#OBJM  = startup.o
#OBJM += system.o

