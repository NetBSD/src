#	$NetBSD: Makefile.inc,v 1.2 1999/03/15 20:54:54 is Exp $

IMAGESIZE?=	5120 # 2.5MB in 512 byte blocks
VND_GEOM?=	512/32/32/5 # (5) 512KB `cylinders'
NEWFSOPTS?=	-i 8192 -B be
