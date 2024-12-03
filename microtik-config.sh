#!/bin/bash

stty -F /dev/ttyS0 115200 cs8 -cstopb -parenb
echo -e "/system reset-configuration\r\n" > /dev/ttyS0
echo -e "y\r\n" > /dev/ttyS0

sleep 60 # change ?

echo -e "admin\r\n" > /dev/ttyS0
sleep 1

echo -e "\r\n" > /dev/ttyS0
sleep 1

echo -e "\r\n" > /dev/ttyS0
sleep 1

echo -e "/interface bridge port remove [find interface=ether1]\r\n" > /dev/ttyS0
sleep 1

echo -e "/interface bridge port remove [find interface=ether2]\r\n" > /dev/ttyS0
sleep 1

echo -e "/interface bridge port remove [find interface=ether3]\r\n" > /dev/ttyS0
sleep 1

echo -e "/interface bridge port remove [find interface=ether4]\r\n" > /dev/ttyS0
sleep 1

echo -e "/interface bridge port remove [find interface=ether5]\r\n" > /dev/ttyS0
sleep 1

echo -e "/interface bridge port remove [find interface=ether6]\r\n" > /dev/ttyS0
sleep 1

echo -e "/interface bridge port remove [find interface=ether7]\r\n" > /dev/ttyS0
sleep 1

echo -e "/interface bridge port remove [find interface=ether8]\r\n" > /dev/ttyS0
sleep 1

echo -e "/interface bridge port remove [find interface=ether9]\r\n" > /dev/ttyS0
sleep 1

echo -e "/interface bridge port remove [find interface=ether10]\r\n" > /dev/ttyS0
sleep 1

echo -e "/interface bridge port remove [find interface=ether11]\r\n" > /dev/ttyS0
sleep 1

echo -e "/interface bridge port remove [find interface=ether12]\r\n" > /dev/ttyS0
sleep 1

echo -e "/interface bridge port remove [find interface=ether13]\r\n" > /dev/ttyS0
sleep 1

echo -e "/interface bridge port remove [find interface=ether14]\r\n" > /dev/ttyS0
sleep 1

echo -e "/interface bridge port remove [find interface=ether15]\r\n" > /dev/ttyS0
sleep 1

echo -e "/interface bridge port remove [find interface=ether16]\r\n" > /dev/ttyS0
sleep 1

echo -e "/interface bridge port remove [find interface=ether17]\r\n" > /dev/ttyS0
sleep 1

echo -e "/interface bridge port remove [find interface=ether18]\r\n" > /dev/ttyS0
sleep 1

echo -e "/interface bridge port remove [find interface=ether19]\r\n" > /dev/ttyS0
sleep 1

echo -e "/interface bridge port remove [find interface=ether20]\r\n" > /dev/ttyS0
sleep 1

echo -e "/interface bridge port remove [find interface=ether21]\r\n" > /dev/ttyS0
sleep 1

echo -e "/interface bridge port remove [find interface=ether22]\r\n" > /dev/ttyS0
sleep 1

echo -e "/interface bridge port remove [find interface=ether23]\r\n" > /dev/ttyS0
sleep 1

echo -e "/interface bridge port remove [find interface=ether24]\r\n" > /dev/ttyS0
sleep 1

echo -e "/interface bridge add name=bridge100\r\n" > /dev/ttyS0
sleep 1

echo -e "/interface bridge add name=bridge101\r\n" > /dev/ttyS0
sleep 1

echo -e "/interface bridge port add bridge=bridge100 interface=ether15\r\n" > /dev/ttyS0
sleep 1

echo -e "/interface bridge port add bridge=bridge100 interface=ether16\r\n" > /dev/ttyS0
sleep 1

echo -e "/interface bridge port add bridge=bridge101 interface=ether22\r\n" > /dev/ttyS0
sleep 1

echo -e "/interface bridge port add bridge=bridge101 interface=ether24\r\n" > /dev/ttyS0
sleep 1