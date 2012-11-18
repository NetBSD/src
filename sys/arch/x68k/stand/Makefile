#	$NetBSD: Makefile,v 1.8.162.2 2012/11/18 19:05:18 riz Exp $

# host tools
SUBDIR= newdisk installboot

# standalone libs
SUBDIR+= libdos libiocs libsa .WAIT

# standalone bootloaders
SUBDIR+= mboot
SUBDIR+= boot_ufs boot_ustar xxboot
SUBDIR+= boot netboot

# Human68k utility
SUBDIR+= aout2hux .WAIT loadbsd

.include <bsd.subdir.mk>
