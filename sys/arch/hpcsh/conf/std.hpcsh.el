# $NetBSD: std.hpcsh.el,v 1.1.2.2 2001/03/27 15:30:58 bouyer Exp $
#
# standard, required NetBSD/hpcsh 'options'
 
machine hpcsh sh3

options	EXEC_SCRIPT	# exec #! scripts

makeoptions	ENDIAN="-EL"
makeoptions 	LDSCRIPTBASE="shl-coff.x"	# for COFF kernel
#makeoptions 	LDSCRIPTBASE="shl-elf.x"	# for ELF kernel
makeoptions 	MACHINE_ARCH=sh3el
