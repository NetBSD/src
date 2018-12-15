#	$NetBSD: Makefile,v 1.4 2018/12/15 18:03:17 gson Exp $

LIVEIMGBASE=	NetBSD-${DISTRIBVER}-sparc-live	# gives ${IMGBASE}.img

.include <bsd.own.mk>

KERN_SET=		kern-GENERIC
.if defined(EMUIMAGEMB)
LIVEIMAGEMB=		${EMUIMAGEMB}
.endif
PRIMARY_BOOT=		bootxx
SECONDARY_BOOT=		boot
SECONDARY_BOOT_ARG=	/${SECONDARY_BOOT}

USE_SUNLABEL=		yes

.include "${.CURDIR}/../../../common/bootimage/Makefile.liveimage"
