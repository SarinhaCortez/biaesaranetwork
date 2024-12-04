#!/bin/bash

stty -F /dev/ttyS0 115200 cs8 -cstopb -parenb

echo -e "/interface bridge port remove [find interface=ether15]\r\n" > /dev/ttyS0
sleep 1

echo -e "/interface bridge port remove [find interface=ether16]\r\n" > /dev/ttyS0
sleep 1

echo -e "/interface bridge port remove [find interface=ether22]\r\n" > /dev/ttyS0
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
