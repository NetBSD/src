#	$NetBSD: Makefile.inc,v 1.2 1999/03/15 20:54:54 is Exp $

VND=		vnd0c
#IMAGESIZE?=	8192 # 512 byte blocks
IMAGESIZE?=	65536 # 512 byte blocks: full 32-meg root.
NEWFSOPTS?=	-c 64 -B le
DISKTYPE?=	miniroot

#
# install bootblock, so that we can boot from there
#
#BOOTINSTALL=${DESTDIR}/usr/mdec/installboot ${DESTDIR}/usr/mdec/xxboot ${VND_RDEV}
PMAX_MDEC_CMD?=-B -b /usr/mdec/rzboot -s /usr/mdec/bootrz
LABEL_CMD?=${DESTDIR}/sbin/disklabel -r -w ${PMAX_MDEC_CMD} ${VND_RDEV} 


# write bootblocks. Update label with fictious name and geometry
# that instructs kernel to recompute  label from # SCSI geometry info
# (since one install method is to dd this onto the start of a disk.)
BOOTINSTALL= ${DESTDIR}/sbin/disklabel -r -R ${PMAX_MDEC_CMD} ${VND_RDEV} \
	${.CURDIR}/../${MACHINE}/miniroot/diskimage.label.proto

#  Write preliminary Label so that newfs succeeds when updating label.
PRELABEL=${LABEL_CMD}  ${DISKTYPE}
PRELABEL=${BOOTINSTALL}
