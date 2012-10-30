#	$NetBSD: Makefile,v 1.1.4.3 2012/10/30 18:48:50 yamt Exp $

LIVEIMGBASE=	NetBSD-${DISTRIBVER}-sparc-live-sd0root	# gives ${IMGBASE}.img

.include <bsd.own.mk>

KERN_SET=		kern-GENERIC
BOOTDISK=		sd0
PRIMARY_BOOT=		bootxx
SECONDARY_BOOT=		boot
SECONDARY_BOOT_ARG=	/${SECONDARY_BOOT}

USE_SUNLABEL=		yes

.include "${.CURDIR}/../../../common/bootimage/Makefile.liveimage"
