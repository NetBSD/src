#	$NetBSD: Makefile.inc,v 1.6 2002/02/12 11:20:38 is Exp $

VND=		vnd0
IMAGESIZE?=	10240 # 512 byte blocks
VND_GEOM?=	512/64/2/80
NEWFSOPTS?=	-c 80 -B be

#
# install bootblock, so that we can boot from there
#
BOOTINSTALL=dd if=${DESTDIR}/usr/mdec/bootxx_ffs of=${VND_RDEV} bs=8192
