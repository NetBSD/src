#	$NetBSD: Makefile,v 1.15 2020/08/16 07:03:45 isaki Exp $

# host tools
SUBDIR= newdisk installboot

# standalone libs
SUBDIR+= libdos libiocs libsa .WAIT

# standalone bootloaders
SUBDIR+= mboot
SUBDIR+= boot_ufs
SUBDIR+= xxboot
SUBDIR+= boot netboot

# Human68k utility
SUBDIR+= aout2hux .WAIT loadbsd

.include <bsd.subdir.mk>
