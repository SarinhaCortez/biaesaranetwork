#!/bin/bash
#===========
#
#mac => 00:c0:df:13:20:1d
#
#===========

ifconfig eth1 172.16.101.1 netmask 255.255.255.0
ifconfig eth2 172.16.100.1 netmask 255.255.255.0


#===========
#
#microtik
#/admin
#/system reset-configuration
#
#===========
