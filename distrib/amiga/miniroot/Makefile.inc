#	$NetBSD: Makefile.inc,v 1.3 2001/09/25 19:21:19 is Exp $

VND=		vnd0c
IMAGESIZE?=	8192 # 512 byte blocks
NEWFSOPTS?=	-c 64 -B be
DISKTYPE?=	miniroot

#
# install bootblock, so that we can boot from there
#
BOOTINSTALL=dd if=${DESTDIR}/usr/mdec/xxboot of=${VND_RDEV} bs=8192
