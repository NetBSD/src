#	$NetBSD: Makefile,v 1.19 2002/02/06 16:15:57 lukem Exp $

BOOTMODEL=	tiny
IMAGE=		ramdisk-${BOOTMODEL}.fs
IMAGESIZE=	1440k
MAKEDEV_DEVS=	std md0 fd0 fd1 wd0 wd1 wd2 wd3 cd0 cd1 pty0 ttyv0
MAKEFS_FLAGS=	-f 10

.include "../common/Makefile.ramdisk"
