#	$NetBSD: Makefile,v 1.1.4.2 2012/04/17 00:02:54 yamt Exp $
LIVEIMGBASE=	vaxlive			# gives ${IMGBASE}.img

.include <bsd.own.mk>

KERN_SET=		kern-GENERIC
BOOTDISK=		ra0
IMAGEMB=		1024		# ra0 can't handle 2GB image
PRIMARY_BOOT=		xxboot
#SECONDARY_BOOT=	# boot is extracted from base.tgz
SECONDARY_BOOT_ARG=	# nothing

.include "${.CURDIR}/../../../common/bootimage/Makefile.liveimage"
