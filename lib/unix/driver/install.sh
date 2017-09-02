#!/bin/bash

if [ -e /dev/gadget ] ; then
echo "adresar /dev/gadget existuje"
else
mkdir /dev/gadget
fi
modprobe udc_core
insmod ./mgadgetfs.ko default_perm=0666
insmod ./dummy_drv.ko
mount -t gadgetfs none /dev/gadget
chmod -R a+rw /dev/gadget
