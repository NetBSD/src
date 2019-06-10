#	$NetBSD: Makefile,v 1.3.32.1 2019/06/10 21:42:28 christos Exp $

LIVEIMGBASE=	NetBSD-${DISTRIBVER}-pmax-live	# gives ${IMGBASE}.img

.include <bsd.own.mk>

KERN_SET=		kern-GENERIC
.if defined(EMUIMAGEMB)
LIVEIMAGEMB=		${EMUIMAGEMB}
.endif
PRIMARY_BOOT=		bootxx_ffs
#SECONDARY_BOOT=	# boot.pmax is extracted from base.tgz
SECONDARY_BOOT_ARG=	# nothing

.include "${.CURDIR}/../../../common/bootimage/Makefile.liveimage"
