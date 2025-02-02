#	$NetBSD: Makefile,v 1.1 2025/02/02 10:35:56 tsutsui Exp $

LIVEIMGBASE=	NetBSD-${DISTRIBVER}-macppc-live	# gives ${IMGBASE}.img

EMUIMAGEMB?=	2048
SWAPMB?=	64
LIVEIMAGEMB=	${EMUIMAGEMB}
PRIMARY_BOOT=	bootxx
SECONDARY_BOOT=	ofwboot
SECONDARY_BOOT_ARG=	/${SECONDARY_BOOT}

.include "${.CURDIR}/../../../common/bootimage/Makefile.liveimage"
