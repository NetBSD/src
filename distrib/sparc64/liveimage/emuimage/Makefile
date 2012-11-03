#	$NetBSD: Makefile,v 1.3 2012/11/03 14:25:33 tsutsui Exp $

LIVEIMGBASE=	NetBSD-${DISTRIBVER}-sparc64-live-sd0root # gives ${IMGBASE}.img

.include <bsd.own.mk>

KERN_SET=		kern-GENERIC
BOOTDISK=		sd0		# for QEMU
#BOOTDISK=		wd0		# for Ultra5/10 image?
.if defined(EMUIMAGEMB)
LIVEIMAGEMB=		${EMUIMAGEMB}
.endif
PRIMARY_BOOT=		bootblk
#SECONDARY_BOOT=	# nothing
#SECONDARY_BOOT_ARG=	# unnecessary

USE_SUNLABEL=		yes

.include "${.CURDIR}/../../../common/bootimage/Makefile.liveimage"
