# $NetBSD: std.evbsh3.el,v 1.3.12.2 2001/11/20 12:56:27 lukem Exp $
#
# standard, required NetBSD/evbsh3 'options'

machine evbsh3 sh3

options 	EXEC_SCRIPT	# exec #! scripts

makeoptions	ENDIAN="-EL"
makeoptions	LDSCRIPTBASE="shl.x"	# for little endian
makeoptions	MACHINE_ARCH=sh3el
