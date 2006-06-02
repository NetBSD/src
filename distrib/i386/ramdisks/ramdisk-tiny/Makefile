#	$NetBSD: Makefile,v 1.1 2006/06/02 22:11:07 hubertf Exp $

BOOTMODEL=	tiny
IMAGE=		ramdisk-${BOOTMODEL}.fs
IMAGESIZE=	1504k
MAKEDEVTARGETS=	std md0 fd0 fd1 wd0 wd1 wd2 wd3 cd0 cd1 pty0 ttyv0
MAKEFS_FLAGS=	-f 10
USE_SYSINST=	en

.include "${.CURDIR}/../common/Makefile.ramdisk"
