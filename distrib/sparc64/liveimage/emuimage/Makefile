#	$NetBSD: Makefile,v 1.4.16.1 2019/06/10 21:42:34 christos Exp $

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
