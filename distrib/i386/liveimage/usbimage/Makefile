#	$NetBSD: Makefile,v 1.1.4.2 2012/04/17 00:02:34 yamt Exp $

LIVEIMGBASE=	i386live		# gives ${IMGBASE}.img

BOOTDISK=	sd0			# for USB flash etc.
IMAGEMB=	1536            	# 2GB USB flash memories aren't 2GiB
OMIT_SWAPIMG=	yes			# no need to write swap

.include "${.CURDIR}/../Makefile.liveimage"
