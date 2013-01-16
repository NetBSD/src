#	$NetBSD: Makefile,v 1.1.4.4 2013/01/16 05:26:01 yamt Exp $

LIVEIMGBASE=	NetBSD-${DISTRIBVER}-i386-live-sd0root	# gives ${IMGBASE}.img

USBIMAGEMB?=	1536            	# 2GB USB flash memories aren't 2GiB

BOOTDISK=	sd0			# for USB flash etc.
LIVEIMAGEMB=	${USBIMAGEMB}
OMIT_SWAPIMG=	yes			# no need to write swap

.include "${.CURDIR}/../Makefile.liveimage"
