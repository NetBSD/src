# $NetBSD: std.evbsh3.el,v 1.3.18.1 2005/11/10 13:56:05 skrll Exp $
#
# standard, required NetBSD/evbsh3 'options'

machine evbsh3 sh3
include		"conf/std"	# MI standard options

options 	EXEC_SCRIPT	# exec #! scripts

makeoptions	ENDIAN="-EL"
makeoptions	LDSCRIPTBASE="shl.x"	# for little endian
makeoptions	MACHINE_ARCH=sh3el
