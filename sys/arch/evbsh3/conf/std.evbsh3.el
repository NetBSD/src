# $NetBSD: std.evbsh3.el,v 1.5.10.1 2006/04/19 02:32:33 elad Exp $
#
# standard, required NetBSD/evbsh3 'options'

machine evbsh3 sh3
include		"conf/std"	# MI standard options

options 	EXEC_SCRIPT	# exec #! scripts

makeoptions	ENDIAN="-EL"
makeoptions	DEFTEXTADDR="0x8c010000"
makeoptions	MACHINE_ARCH=sh3el
