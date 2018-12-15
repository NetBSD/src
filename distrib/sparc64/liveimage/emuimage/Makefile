#	$NetBSD: Makefile,v 1.5 2018/12/15 18:03:18 gson Exp $

LIVEIMGBASE=	NetBSD-${DISTRIBVER}-sparc64-live # gives ${IMGBASE}.img

.include <bsd.own.mk>

KERN_SET=		kern-GENERIC
.if defined(EMUIMAGEMB)
LIVEIMAGEMB=		${EMUIMAGEMB}
.endif
PRIMARY_BOOT=		bootblk
SECONDARY_BOOT=		ofwboot
#SECONDARY_BOOT_ARG=	# unnecessary

USE_SUNLABEL=		yes

.include "${.CURDIR}/../../../common/bootimage/Makefile.liveimage"
