#	$NetBSD: Makefile,v 1.4.10.1 2025/02/22 12:59:26 martin Exp $

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

# sun2 doesn't have shared libraries so omit comp etc. to reduce image size.
SETS=		modules base etc games misc text

.include "${.CURDIR}/../../../common/bootimage/Makefile.liveimage"
