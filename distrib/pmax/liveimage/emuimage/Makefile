#	$NetBSD: Makefile,v 1.1.4.2 2012/04/17 00:02:37 yamt Exp $
LIVEIMGBASE=	pmaxlive			# gives ${IMGBASE}.img

.include <bsd.own.mk>

KERN_SET=		kern-GENERIC
BOOTDISK=		sd0
PRIMARY_BOOT=		bootxx_ffs
#SECONDARY_BOOT=	# boot.pmax is extracted from base.tgz
SECONDARY_BOOT_ARG=	# nothing

.include "${.CURDIR}/../../../common/bootimage/Makefile.liveimage"
