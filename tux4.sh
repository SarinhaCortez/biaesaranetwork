#!/bin/bash

#===========================================================================
#eth1: MAC => 00:c0:df:13:20:10
#==========================================================================


ifconfig eth1 172.16.40.254/24 netmask 255.255.255.0
route -n
arp -a


#=============================
#
#
#/interface bridge add name=bridge40 
#/interface bridge print
#
#=============================
