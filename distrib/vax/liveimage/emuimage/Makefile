#	$NetBSD: Makefile,v 1.1.2.1 2012/08/17 20:50:50 martin Exp $

LIVEIMGBASE=	NetBSD-${DISTRIBVER}-vax-live-ra0root	# gives ${IMGBASE}.img

.include <bsd.own.mk>

KERN_SET=		kern-GENERIC
BOOTDISK=		ra0
IMAGEMB=		1024		# ra0 can't handle 2GB image
PRIMARY_BOOT=		xxboot
#SECONDARY_BOOT=	# boot is extracted from base.tgz
SECONDARY_BOOT_ARG=	# nothing

.include "${.CURDIR}/../../../common/bootimage/Makefile.liveimage"
