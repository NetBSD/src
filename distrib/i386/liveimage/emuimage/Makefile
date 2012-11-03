#	$NetBSD: Makefile,v 1.3 2012/11/03 14:25:33 tsutsui Exp $

LIVEIMGBASE=	NetBSD-${DISTRIBVER}-i386-live-wd0root	# gives ${IMGBASE}.img

BOOTDISK=	wd0			# for QEMU etc.
.if defined(EMUIMAGEMB)
LIVEIMAGEMB=	${EMUIMAGEMB}
.endif

.include "${.CURDIR}/../Makefile.liveimage"
