#	$NetBSD: Makefile,v 1.3.10.1 2015/05/02 18:09:06 martin Exp $

LIVEIMGBASE=	NetBSD-${DISTRIBVER}-sparc64-live-sd0root # gives ${IMGBASE}.img

.include <bsd.own.mk>

KERN_SET=		kern-GENERIC
BOOTDISK=		sd0		# for QEMU
#BOOTDISK=		wd0		# for Ultra5/10 image?
.if defined(EMUIMAGEMB)
LIVEIMAGEMB=		${EMUIMAGEMB}
.endif
PRIMARY_BOOT=		bootblk
SECONDARY_BOOT=		ofwboot
#SECONDARY_BOOT_ARG=	# unnecessary

USE_SUNLABEL=		yes

.include "${.CURDIR}/../../../common/bootimage/Makefile.liveimage"
