#!/bin/bash

#===========================================================================
#eth1: MAC => 00:e0:7d:c8:7c:55 
#==========================================================================


ifconfig eth1 172.16.100.1/24 netmask 255.255.255.0
route -n
arp -a
arp -d 172.16.100.254
arp -a

