# $NetBSD: std.hpcsh.el,v 1.1 2001/01/17 05:21:48 itojun Exp $
#
# standard, required NetBSD/hpcsh 'options'
 
machine hpcsh sh3

options	EXEC_SCRIPT	# exec #! scripts

makeoptions	ENDIAN="-EL"
makeoptions 	LDSCRIPTBASE="shl.x"	# for little endian
makeoptions 	MACHINE_ARCH=sh3el
