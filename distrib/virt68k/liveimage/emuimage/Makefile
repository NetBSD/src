#	$NetBSD: Makefile,v 1.1 2025/01/19 04:58:43 tsutsui Exp $

LIVEIMGBASE=	NetBSD-${DISTRIBVER}-virt68k-live	# gives ${IMGBASE}.img

.include <bsd.own.mk>

KERN_SET=		kern-GENERIC
.if defined(EMUIMAGEMB)
LIVEIMAGEMB=		${EMUIMAGEMB}
.endif

.include "${.CURDIR}/../../../common/bootimage/Makefile.liveimage"
