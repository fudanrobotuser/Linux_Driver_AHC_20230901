#!/bin/sh
module=""
device=""
mode="664"
maxports="32"

echo -e "Load module"
insmod gpio-ahc0b10.ko

