#	$NetBSD: Makefile.inc,v 1.5 2001/12/17 05:59:20 mhitch Exp $

VND=		vnd0
IMAGESIZE?=	8192 # 512 byte blocks
NEWFSOPTS?=	-c 64 -B be
DISKTYPE?=	miniroot

#
# install bootblock, so that we can boot from there
#
BOOTINSTALL=dd if=${DESTDIR}/usr/mdec/bootxx_ffs of=${VND_RDEV} bs=8192
