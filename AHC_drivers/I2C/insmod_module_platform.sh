#!/bin/sh
module=""
device=""
mode="664"
maxports="32"

echo -e "Load I2C module and I2C-dev module"
modprobe i2c-core
modprobe i2c-dev

echo -e "Load RDC_I2C_platform driver"
insmod i2c-ahc0510.ko
