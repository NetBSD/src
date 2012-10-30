#	$NetBSD: Makefile,v 1.1.44.1 2012/10/30 18:48:33 yamt Exp $

BOOTMODEL=	tiny
IMAGE=		ramdisk-rescue${BOOTMODEL}.fs
IMAGESIZE=	1500k
MAKEDEVTARGETS=	std md0 fd0 fd1 wd0 wd1 wd2 wd3 cd0 cd1 ipty ttyv0
MAKEFS_FLAGS=	-f 20
STRIP=${OBJCOPY} -S -R .ident -R .note

.include "${.CURDIR}/../common/Makefile.ramdisk"
