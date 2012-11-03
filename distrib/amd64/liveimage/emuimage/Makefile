#	$NetBSD: Makefile,v 1.3 2012/11/03 14:25:32 tsutsui Exp $

LIVEIMGBASE=	NetBSD-${DISTRIBVER}-amd64-live-wd0root	# gives ${IMGBASE}.img

BOOTDISK=	wd0			# for QEMU etc.
.if defined(EMUIMAGEMB)
LIVEIMAGEMB=	${EMUIMAGEMB}
.endif

.include "${.CURDIR}/../Makefile.liveimage"
