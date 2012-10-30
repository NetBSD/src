#	$NetBSD: Makefile,v 1.8.156.2 2012/10/30 17:20:31 yamt Exp $

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
