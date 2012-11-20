#	$NetBSD: Makefile,v 1.11.2.1 2012/11/20 03:01:49 tls Exp $

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
