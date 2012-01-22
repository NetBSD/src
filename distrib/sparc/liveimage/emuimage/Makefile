#	$NetBSD: Makefile,v 1.1 2012/01/22 03:53:31 tsutsui Exp $

LIVEIMGBASE=	sparcemu		# gives ${IMGBASE}.img

.include <bsd.own.mk>

KERN_SET=		kern-GENERIC
BOOTDISK=		sd0
PRIMARY_BOOT=		bootxx
SECONDARY_BOOT=		boot
SECONDARY_BOOT_ARG=	/${SECONDARY_BOOT}

USE_SUNLABEL=		yes

.include "${.CURDIR}/../../../common/bootimage/Makefile.liveimage"
