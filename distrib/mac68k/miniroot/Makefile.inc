#	$NetBSD: Makefile.inc,v 1.2.10.1 2001/06/25 20:35:36 he Exp $

IMAGESIZE?=	5120 # 2.5MB in 512 byte blocks
VND_GEOM?=	512/32/32/5 # (5) 512KB `cylinders'
NEWFSOPTS?=	-i 2048 -B be
