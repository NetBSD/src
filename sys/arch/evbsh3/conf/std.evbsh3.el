# $NetBSD: std.evbsh3.el,v 1.2 2001/01/04 07:03:23 itojun Exp $
#
# standard, required NetBSD/evbsh3 'options'
 
machine evbsh3 sh3

options	EXEC_SCRIPT	# exec #! scripts

makeoptions	ENDIAN="-EL"
makeoptions 	LDSCRIPTBASE="shl.x"	# for little endian
makeoptions 	MACHINE_ARCH=sh3el
