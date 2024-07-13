#!/bin/sh
module=""
device=""
mode="664"
maxports="32"

echo -e "Unload RDC_SMB_platform driver"
rmmod smb-ahc0511
