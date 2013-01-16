#	$NetBSD: Makefile,v 1.1.4.4 2013/01/16 05:26:11 yamt Exp $

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
