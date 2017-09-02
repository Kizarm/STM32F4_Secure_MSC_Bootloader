# Modul existuje v distribuci jako /drivers/usb/gadget/dummy_hcd.ko
# V novém Ubuntu už ne. Vezmeme dummy_hcd.c z aktualni verze jadra.
# Patch jen nastavi emulovanou rychlost na FS. Lze udelat i rucne,
# v UBUNTU 16.04 je chyba v inode.c, nechodi to, ale zde (snad) opraveno,
# alespon to nejak funguje. Ve starsim je mozne pouzit puvodni gadgetfs.ko.
# Ve verzi jadra 4.4 je to chybne, ve verzi 4.9 uz opraveno.

# Do tohoto adresare zkopirujte dummy_hcd.c inode.c z aktualni verze jadra
# je to nekde v /drivers/usb/gadget a pouzijte patch -p2 <gadget.patch

obj-m          += dummy_drv.o
obj-m          += mgadgetfs.o
dummy_drv-objs := dummy_hcd.o
mgadgetfs-objs := inode.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

default: dummy_hcd.c inode.c
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

clean:
	-rm -f modules.order Module.* *.o .*.cmd .*.flags *.mod.c *~
	-rm -rf .tmp_versions
