#	$NetBSD: Makefile,v 1.17 2002/02/06 16:15:56 lukem Exp $

BOOTMODEL=	tiny
IMAGE=		ramdisk-rescue${BOOTMODEL}.fs
IMAGESIZE=	1440k
MAKEDEV_DEVS=	std md0 fd0 fd1 wd0 wd1 wd2 wd3 cd0 cd1 pty0 ttyv0
MAKEFS_FLAGS=	-f 10

.include "../common/Makefile.ramdisk"
