#	$NetBSD: Makefile,v 1.2 2012/08/14 07:24:41 martin Exp $

BOOTMODEL=	tiny
IMAGE=		ramdisk-${BOOTMODEL}.fs
IMAGESIZE=	1504k
MAKEDEVTARGETS=	std md0 fd0 fd1 wd0 wd1 wd2 wd3 cd0 cd1 ipty ttyv0
MAKEFS_FLAGS=	-f 10
USE_SYSINST=	en

.include "${.CURDIR}/../common/Makefile.ramdisk"
