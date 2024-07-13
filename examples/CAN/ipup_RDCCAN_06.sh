#!/bin/sh
module="RDC_CAN_17f3"
device="RDCCAN"
mode="664"
maxports="32"

echo -e ""
echo -e "-------------------------------------------"
echo -e "check if there are a can0 and a can1 device."
echo -e ""
ip link show

echo -e ""
echo -e "-------------------------------------------"
echo -e "show setting information of a can device"
echo -e ""
ip link set can0 type can help

ip link set can0 type can bitrate 1000000 sample-point 0.6 restart-ms 1
ip link set can1 type can bitrate 1000000 sample-point 0.6 restart-ms 1
#ip link set can0 type can tq 40 prop-seg 8 phase-seg1 8 phase-seg2 8
#ip link set can1 type can tq 40 prop-seg 8 phase-seg1 8 phase-seg2 8
#ip link set can0 type can tq 200 prop-seg 1 phase-seg1 4 phase-seg2 4

echo -e ""
echo -e "-------------------------------------------"
echo -e "show statistic information of can0 and can1"
echo -e ""
ip -details -statistics link show can0
ip -details -statistics link show can1

echo -e ""
echo -e "-------------------------------------------"
echo -e "ip link set can0 up type can"
echo -e "type ifconfig to see can0"

ip link set can0 up type can
ip link set can1 up type can
