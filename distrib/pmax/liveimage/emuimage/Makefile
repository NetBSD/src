#	$NetBSD: Makefile,v 1.1.2.1 2012/08/17 20:50:50 martin Exp $

LIVEIMGBASE=	NetBSD-${DISTRIBVER}-pmax-live-sd0root	# gives ${IMGBASE}.img

.include <bsd.own.mk>

KERN_SET=		kern-GENERIC
BOOTDISK=		sd0
PRIMARY_BOOT=		bootxx_ffs
#SECONDARY_BOOT=	# boot.pmax is extracted from base.tgz
SECONDARY_BOOT_ARG=	# nothing

.include "${.CURDIR}/../../../common/bootimage/Makefile.liveimage"
