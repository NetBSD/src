#	$NetBSD: Makefile,v 1.3 2012/11/03 14:25:32 tsutsui Exp $

LIVEIMGBASE=	NetBSD-${DISTRIBVER}-amd64-live-sd0root	# gives ${IMGBASE}.img

USBIMAGEMB?=	1536            	# 2GB USB flash memories aren't 2GiB

BOOTDISK=	sd0			# for USB flash etc.
LIVEIMAGEMB=	${USBIMAGEMB}
OMIT_SWAPIMG=	yes			# no need to write swap

.include "${.CURDIR}/../Makefile.liveimage"
