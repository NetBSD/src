# $NetBSD: std.evbsh3.el,v 1.5.12.1 2006/03/28 09:47:15 tron Exp $
#
# standard, required NetBSD/evbsh3 'options'

machine evbsh3 sh3
include		"conf/std"	# MI standard options

options 	EXEC_SCRIPT	# exec #! scripts

makeoptions	ENDIAN="-EL"
makeoptions	DEFTEXTADDR="0x8c010000"
makeoptions	MACHINE_ARCH=sh3el
