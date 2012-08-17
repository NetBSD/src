#	$NetBSD: Makefile,v 1.1.2.1 2012/08/17 20:50:50 martin Exp $

LIVEIMGBASE=	NetBSD-${DISTRIBVER}-amd64-live-sd0root	# gives ${IMGBASE}.img

BOOTDISK=	sd0			# for USB flash etc.
IMAGEMB=	1536            	# 2GB USB flash memories aren't 2GiB
OMIT_SWAPIMG=	yes			# no need to write swap

.include "${.CURDIR}/../Makefile.liveimage"
