#	$NetBSD: Makefile,v 1.1 2025/02/02 20:11:02 tsutsui Exp $

LIVEIMGBASE=	NetBSD-${DISTRIBVER}-alpha-live	# gives ${IMGBASE}.img

.include <bsd.own.mk>

KERN_SET=		kern-GENERIC
.if defined(EMUIMAGEMB)
LIVEIMAGEMB=		${EMUIMAGEMB}
.endif
IMGFFSVERSION=		2
PRIMARY_BOOT=		bootxx_ffsv2
SECONDARY_BOOT=		boot

.include "${.CURDIR}/../../../common/bootimage/Makefile.liveimage"
