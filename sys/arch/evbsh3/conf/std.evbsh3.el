# $NetBSD: std.evbsh3.el,v 1.2.6.1 2002/01/10 19:42:43 thorpej Exp $
#
# standard, required NetBSD/evbsh3 'options'

machine evbsh3 sh3

options 	EXEC_SCRIPT	# exec #! scripts

makeoptions	ENDIAN="-EL"
makeoptions	LDSCRIPTBASE="shl.x"	# for little endian
makeoptions	MACHINE_ARCH=sh3el
