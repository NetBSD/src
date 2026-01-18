#	$NetBSD: Makefile,v 1.7 2026/01/18 07:59:28 tsutsui Exp $

LIVEIMGBASE=	NetBSD-${DISTRIBVER}-vax-live	# gives ${IMGBASE}.img

.include <bsd.own.mk>

# MSCP RA92 disk (suggested in the "NetBSD SIMH HOW-TO" page) has
# 2,940,951 blocks (1,505,766,912 bytes ~= 1436 MB)
EMUIMAGEMB?=		1436

KERN_SET=		kern-GENERIC
LIVEIMAGEMB=		${EMUIMAGEMB}
PRIMARY_BOOT=		xxboot
#SECONDARY_BOOT=	# boot is extracted from base.tgz
SECONDARY_BOOT_ARG=	# nothing

# XXX: tools disklabel(8) for vax doesn't preserve existing bootblock?
INSTALLBOOT_AFTER_DISKLABEL=	yes

.include "${.CURDIR}/../../../common/bootimage/Makefile.liveimage"
