#	$NetBSD: Makefile,v 1.20 2002/02/08 05:02:20 lukem Exp $

BOOTMODEL=	tiny
IMAGE=		ramdisk-${BOOTMODEL}.fs
IMAGESIZE=	1440k
MAKEDEVTARGETS=	std md0 fd0 fd1 wd0 wd1 wd2 wd3 cd0 cd1 pty0 ttyv0
MAKEFS_FLAGS=	-f 10
USE_SYSINST=	yes

.include "${.CURDIR}/../common/Makefile.ramdisk"
