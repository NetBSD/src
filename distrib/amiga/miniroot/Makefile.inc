#	$NetBSD: Makefile.inc,v 1.1 1997/09/09 23:07:03 is Exp $

VND=		vnd0c
IMAGESIZE?=	8192 # 512 byte blocks
NEWFSOPTS?=	-c 64
DISKTYPE?=	miniroot

#
# install bootblock, so that we can boot from there
#
BOOTINSTALL=${DESTDIR}/usr/mdec/installboot ${DESTDIR}/usr/mdec/xxboot ${VND_RDEV}
