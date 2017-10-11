CROSS_COMPILE = arm-none-eabi-
CPU_TYPE      = cortex-m4
MFLAGS        = -mthumb -mcpu=$(CPU_TYPE)

VPATH += $(ROOT_PATH)/src
VPATH += $(ROOT_PATH)/lib/$(TARGET)/src

LDFLAGS  += -nostartfiles
#LDFLAGS  += -lc --specs=nosys.specs
#LD_SCRIPT = -T stm32_ram.ld
LD_SCRIPT = -T stm32_flash.ld

#OBJM  =
OBJM += startup.o
OBJM += system.o

