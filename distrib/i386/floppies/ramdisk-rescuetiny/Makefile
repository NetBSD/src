#	$NetBSD: Makefile,v 1.19 2002/12/20 23:11:59 thorpej Exp $

BOOTMODEL=	tiny
IMAGE=		ramdisk-rescue${BOOTMODEL}.fs
IMAGESIZE=	1460k
MAKEDEVTARGETS=	std md0 fd0 fd1 wd0 wd1 wd2 wd3 cd0 cd1 pty0 ttyv0
MAKEFS_FLAGS=	-f 20

.include "${.CURDIR}/../common/Makefile.ramdisk"
