#!/bin/sh
module=""
device=""
mode="664"
maxports="32"

echo -e "Unload serial_ahc_A9610 driver"
rmmod com-ahc0501
