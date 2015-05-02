#	$NetBSD: Makefile,v 1.1.2.1.6.1 2015/05/02 17:06:01 martin Exp $

LIVEIMGBASE=	NetBSD-${DISTRIBVER}-sparc64-live-sd0root # gives ${IMGBASE}.img

.include <bsd.own.mk>

KERN_SET=		kern-GENERIC
BOOTDISK=		sd0		# for QEMU
#BOOTDISK=		wd0		# for Ultra5/10 image?
PRIMARY_BOOT=		bootblk
SECONDARY_BOOT=		ofwboot
#SECONDARY_BOOT_ARG=	# unnecessary

USE_SUNLABEL=		yes

.include "${.CURDIR}/../../../common/bootimage/Makefile.liveimage"
