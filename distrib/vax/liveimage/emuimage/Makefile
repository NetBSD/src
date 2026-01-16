#	$NetBSD: Makefile,v 1.6 2026/01/16 12:15:48 tsutsui Exp $

LIVEIMGBASE=	NetBSD-${DISTRIBVER}-vax-live	# gives ${IMGBASE}.img

.include <bsd.own.mk>

EMUIMAGEMB?=		1024		# open-simh ra0 can't handle >1GB image
SWAPMB?=		64

# Exclude man etc. due to size restriction
KERN_SET=	kern-GENERIC
SETS=		modules base comp etc misc rescue
.if ${MKX11} != "no"
SETS+=		xbase xetc xfont xserver
.endif

KERN_SET=		kern-GENERIC
LIVEIMAGEMB=		${EMUIMAGEMB}
PRIMARY_BOOT=		xxboot
#SECONDARY_BOOT=	# boot is extracted from base.tgz
SECONDARY_BOOT_ARG=	# nothing

# XXX: tools disklabel(8) for vax doesn't preserve existing bootblock?
INSTALLBOOT_AFTER_DISKLABEL=	yes

.include "${.CURDIR}/../../../common/bootimage/Makefile.liveimage"
