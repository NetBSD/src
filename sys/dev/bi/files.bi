#	$NetBSD: files.bi,v 1.1.2.1 2000/11/20 11:39:51 bouyer Exp $
#
# Config file and device description for machine-independent
# code for devices Digital Equipment Corp. BI bus.
# Included by ports that need it.

device	bi { node=-1 }

file	dev/bi/bi.c				bi

# KDB50 on BI
device	kdb: mscp
attach	kdb at bi
file	dev/bi/kdb.c				kdb

# DEBNA/DEBNT Ethernet Adapter
device	ni: ifnet, ether, arp
attach	ni at bi
file	dev/bi/if_ni.c				ni

# DWBUA BI-Unibus adapter
attach	uba at bi with uba_bi
file	dev/bi/uba_bi.c				uba_bi
