#	$NetBSD: Makefile,v 1.3.32.1 2019/06/10 21:42:38 christos Exp $

LIVEIMGBASE=	NetBSD-${DISTRIBVER}-vax-live	# gives ${IMGBASE}.img

.include <bsd.own.mk>

EMUIMAGEMB?=		1024		# ra0 can't handle 2GB image

KERN_SET=		kern-GENERIC
LIVEIMAGEMB=		${EMUIMAGEMB}
PRIMARY_BOOT=		xxboot
#SECONDARY_BOOT=	# boot is extracted from base.tgz
SECONDARY_BOOT_ARG=	# nothing

.include "${.CURDIR}/../../../common/bootimage/Makefile.liveimage"
