#	$NetBSD: Makefile.inc,v 1.4 2000/01/06 03:53:47 simonb Exp $

VND=		vnd0c
#IMAGESIZE?=	8192 # 512 byte blocks
IMAGESIZE?=	65536 # 512 byte blocks: full 32-meg root.
NEWFSOPTS?=	-c 32 -B le
DISKTYPE?=	miniroot
LABELPROTO?=	${.CURDIR}/../${MACHINE}/miniroot/diskimage.label.proto

#
# install bootblock, so that we can boot from there
#
BOOTINSTALL= ${DESTDIR}/usr/mdec/installboot ${VND_RDEV} \
	${DESTDIR}/usr/mdec/bootxx_ffs

# Write preliminary Label so that newfs succeeds when updating label.
PRELABEL=disklabel -rw ${VND_RDEV} ${DISKTYPE}
