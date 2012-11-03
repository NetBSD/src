#	$NetBSD: Makefile,v 1.3 2012/11/03 14:25:33 tsutsui Exp $

LIVEIMGBASE=	NetBSD-${DISTRIBVER}-sun3-live-sd0root	# gives ${IMGBASE}.img

.include <bsd.own.mk>

KERN_SET=		kern-GENERIC
BOOTDISK=		sd0
.if defined(EMUIMAGEMB)
LIVEIMAGEMB=		${EMUIMAGEMB}
.endif
PRIMARY_BOOT=		bootxx
SECONDARY_BOOT=		ufsboot
SECONDARY_BOOT_ARG=	/${SECONDARY_BOOT}

USE_SUNLABEL=		yes

.include "${.CURDIR}/../../../common/bootimage/Makefile.liveimage"
