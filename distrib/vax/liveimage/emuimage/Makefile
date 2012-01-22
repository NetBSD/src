#	$NetBSD: Makefile,v 1.1 2012/01/22 03:53:32 tsutsui Exp $
LIVEIMGBASE=	vaxlive			# gives ${IMGBASE}.img

.include <bsd.own.mk>

KERN_SET=		kern-GENERIC
BOOTDISK=		ra0
IMAGEMB=		1024		# ra0 can't handle 2GB image
PRIMARY_BOOT=		xxboot
#SECONDARY_BOOT=	# boot is extracted from base.tgz
SECONDARY_BOOT_ARG=	# nothing

.include "${.CURDIR}/../../../common/bootimage/Makefile.liveimage"
