#	$NetBSD: Makefile,v 1.3 2012/11/03 14:25:33 tsutsui Exp $

LIVEIMGBASE=	NetBSD-${DISTRIBVER}-vax-live-ra0root	# gives ${IMGBASE}.img

.include <bsd.own.mk>

EMUIMAGEMB?=		1024		# ra0 can't handle 2GB image

KERN_SET=		kern-GENERIC
BOOTDISK=		ra0
LIVEIMAGEMB=		${EMUIMAGEMB}
PRIMARY_BOOT=		xxboot
#SECONDARY_BOOT=	# boot is extracted from base.tgz
SECONDARY_BOOT_ARG=	# nothing

.include "${.CURDIR}/../../../common/bootimage/Makefile.liveimage"
