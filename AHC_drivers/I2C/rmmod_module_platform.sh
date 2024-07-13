#!/bin/sh
module=""
device=""
mode="664"
maxports="32"

echo -e "Unload RDC_I2C_platform driver"
rmmod i2c-ahc0510
