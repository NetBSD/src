#	$NetBSD: Makefile,v 1.20 2003/02/24 18:59:26 briggs Exp $

BOOTMODEL=	tiny
IMAGE=		ramdisk-rescue${BOOTMODEL}.fs
IMAGESIZE=	1480k
MAKEDEVTARGETS=	std md0 fd0 fd1 wd0 wd1 wd2 wd3 cd0 cd1 pty0 ttyv0
MAKEFS_FLAGS=	-f 20

.include "${.CURDIR}/../common/Makefile.ramdisk"
