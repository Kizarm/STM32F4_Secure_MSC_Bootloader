CROSS_COMPILE = arm-none-eabi-
CPU_TYPE      = cortex-m4
MFLAGS        = -mthumb -mcpu=$(CPU_TYPE)

VPATH += $(ROOT_PATH)/src
VPATH += $(ROOT_PATH)/lib/$(TARGET)/src

LDFLAGS  += -nostartfiles
LD_SCRIPT = -T user_ram.ld

OBJM  =
OBJM  = user.o
OBJM += system.o

