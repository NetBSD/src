# $NetBSD: std.evbsh3.el,v 1.4 2005/09/17 09:44:06 yamt Exp $
#
# standard, required NetBSD/evbsh3 'options'

machine evbsh3 sh3
include		"conf/std"	# MI standard options

options 	EXEC_SCRIPT	# exec #! scripts

makeoptions	ENDIAN="-EL"
makeoptions	LDSCRIPTBASE="shl.x"	# for little endian
makeoptions	MACHINE_ARCH=sh3el
