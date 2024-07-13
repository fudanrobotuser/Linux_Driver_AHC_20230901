#!/bin/sh
module=""
device=""
mode="664"
maxports="32"

echo -e "Unload module"
rmmod gpio-ahc0b10
