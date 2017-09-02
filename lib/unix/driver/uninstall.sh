#/bin/bash

umount /dev/gadget
rmmod mgadgetfs
rmmod dummy_drv
rmmod udc_core
