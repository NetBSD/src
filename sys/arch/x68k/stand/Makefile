#	$NetBSD: Makefile,v 1.14 2020/08/14 03:29:23 isaki Exp $

# host tools
SUBDIR= newdisk installboot

# standalone libs
SUBDIR+= libdos libiocs libsa .WAIT

# standalone bootloaders
SUBDIR+= mboot
SUBDIR+= boot_ufs
SUBDIR+= boot_ustar
SUBDIR+= xxboot
SUBDIR+= boot netboot

# Human68k utility
SUBDIR+= aout2hux .WAIT loadbsd

.include <bsd.subdir.mk>
