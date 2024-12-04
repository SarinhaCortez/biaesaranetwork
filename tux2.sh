#!/bin/bash
#===========
#
#mac => 00:c0:df:13:20:1d
#
#===========

ifconfig eth1 172.16.101.1 netmask 255.255.255.0


route -n
arp -a


#===========
#
#microtik
#/admin
#/system reset-configuration
#
#===========
