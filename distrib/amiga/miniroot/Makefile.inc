#	$NetBSD: Makefile.inc,v 1.2 1999/03/15 20:54:54 is Exp $

VND=		vnd0c
IMAGESIZE?=	8192 # 512 byte blocks
NEWFSOPTS?=	-c 64 -B be
DISKTYPE?=	miniroot

#
# install bootblock, so that we can boot from there
#
BOOTINSTALL=${DESTDIR}/usr/mdec/installboot ${DESTDIR}/usr/mdec/xxboot ${VND_RDEV}
