#!/bin/sh
module=""
device=""
mode="664"
maxports="32"

echo -e "Load parport module and serial module"
modprobe parport
modprobe serial

echo -e "Load serial_ahc_A9610 driver"
insmod com-ahc0501.ko
