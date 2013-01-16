#	$NetBSD: Makefile,v 1.1.4.4 2013/01/16 05:26:00 yamt Exp $

LIVEIMGBASE=	NetBSD-${DISTRIBVER}-amd64-live-wd0root	# gives ${IMGBASE}.img

BOOTDISK=	wd0			# for QEMU etc.
.if defined(EMUIMAGEMB)
LIVEIMAGEMB=	${EMUIMAGEMB}
.endif

.include "${.CURDIR}/../Makefile.liveimage"
