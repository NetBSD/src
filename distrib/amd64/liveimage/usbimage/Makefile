#	$NetBSD: Makefile,v 1.1.4.3 2012/10/30 18:48:28 yamt Exp $

LIVEIMGBASE=	NetBSD-${DISTRIBVER}-amd64-live-sd0root	# gives ${IMGBASE}.img

BOOTDISK=	sd0			# for USB flash etc.
IMAGEMB=	1536            	# 2GB USB flash memories aren't 2GiB
OMIT_SWAPIMG=	yes			# no need to write swap

.include "${.CURDIR}/../Makefile.liveimage"
