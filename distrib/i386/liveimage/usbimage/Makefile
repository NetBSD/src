#	$NetBSD: Makefile,v 1.2 2012/08/17 16:56:35 riz Exp $

LIVEIMGBASE=	NetBSD-${DISTRIBVER}-i386-live-sd0root	# gives ${IMGBASE}.img

BOOTDISK=	sd0			# for USB flash etc.
IMAGEMB=	1536            	# 2GB USB flash memories aren't 2GiB
OMIT_SWAPIMG=	yes			# no need to write swap

.include "${.CURDIR}/../Makefile.liveimage"
