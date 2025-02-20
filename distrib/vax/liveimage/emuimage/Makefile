#	$NetBSD: Makefile,v 1.4.10.1 2025/02/20 19:13:29 martin Exp $

LIVEIMGBASE=	NetBSD-${DISTRIBVER}-vax-live	# gives ${IMGBASE}.img

.include <bsd.own.mk>

EMUIMAGEMB?=		1024		# ra0 can't handle 2GB image

KERN_SET=		kern-GENERIC
LIVEIMAGEMB=		${EMUIMAGEMB}
PRIMARY_BOOT=		xxboot
#SECONDARY_BOOT=	# boot is extracted from base.tgz
SECONDARY_BOOT_ARG=	# nothing

# XXX: tools disklabel(8) for vax doesn't preserve existing bootblock?
INSTALLBOOT_AFTER_DISKLABEL=	yes

.include "${.CURDIR}/../../../common/bootimage/Makefile.liveimage"
