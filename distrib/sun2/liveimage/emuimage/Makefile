#	$NetBSD: Makefile,v 1.4 2018/12/15 18:03:18 gson Exp $

LIVEIMGBASE=	NetBSD-${DISTRIBVER}-sun2-live	# gives ${IMGBASE}.img

.include <bsd.own.mk>

KERN_SET=		kern-GENERIC
.if defined(EMUIMAGEMB)
LIVEIMAGEMB=		${EMUIMAGEMB}
.endif
PRIMARY_BOOT=		bootxx
SECONDARY_BOOT=		ufsboot
SECONDARY_BOOT_ARG=	/${SECONDARY_BOOT}

#USE_SUNLABEL=		yes		# sun2 PROM doesn't read sunlabel

.include "${.CURDIR}/../../../common/bootimage/Makefile.liveimage"
