#	$NetBSD: Makefile,v 1.21 2003/06/05 14:30:12 dsl Exp $

BOOTMODEL=	tiny
IMAGE=		ramdisk-rescue${BOOTMODEL}.fs
IMAGESIZE=	1500k
MAKEDEVTARGETS=	std md0 fd0 fd1 wd0 wd1 wd2 wd3 cd0 cd1 pty0 ttyv0
MAKEFS_FLAGS=	-f 20
STRIP=${OBJCOPY} -S -R .ident -R .note

.include "${.CURDIR}/../common/Makefile.ramdisk"
