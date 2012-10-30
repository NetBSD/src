#	$NetBSD: Makefile,v 1.1.4.3 2012/10/30 18:48:51 yamt Exp $

LIVEIMGBASE=	NetBSD-${DISTRIBVER}-sparc64-live-sd0root # gives ${IMGBASE}.img

.include <bsd.own.mk>

KERN_SET=		kern-GENERIC
BOOTDISK=		sd0		# for QEMU
#BOOTDISK=		wd0		# for Ultra5/10 image?
PRIMARY_BOOT=		bootblk
#SECONDARY_BOOT=	# nothing
#SECONDARY_BOOT_ARG=	# unnecessary

USE_SUNLABEL=		yes

.include "${.CURDIR}/../../../common/bootimage/Makefile.liveimage"
