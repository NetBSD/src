#	$NetBSD: Makefile.inc,v 1.1 1997/09/22 23:37:19 scottr Exp $

IMAGESIZE?=	5120 # 2.5MB in 512 byte blocks
VND_GEOM?=	512/32/32/5 # (5) 512KB `cylinders'
NEWFSOPTS?=	-i 8192
