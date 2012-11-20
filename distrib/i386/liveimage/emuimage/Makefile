#	$NetBSD: Makefile,v 1.2.2.1 2012/11/20 02:57:42 tls Exp $

LIVEIMGBASE=	NetBSD-${DISTRIBVER}-i386-live-wd0root	# gives ${IMGBASE}.img

BOOTDISK=	wd0			# for QEMU etc.
.if defined(EMUIMAGEMB)
LIVEIMAGEMB=	${EMUIMAGEMB}
.endif

.include "${.CURDIR}/../Makefile.liveimage"
