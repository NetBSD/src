#	$NetBSD: Makefile,v 1.1 2012/01/22 03:53:30 tsutsui Exp $
LIVEIMGBASE=	pmaxlive			# gives ${IMGBASE}.img

.include <bsd.own.mk>

KERN_SET=		kern-GENERIC
BOOTDISK=		sd0
PRIMARY_BOOT=		bootxx_ffs
#SECONDARY_BOOT=	# boot.pmax is extracted from base.tgz
SECONDARY_BOOT_ARG=	# nothing

.include "${.CURDIR}/../../../common/bootimage/Makefile.liveimage"
