#	$NetBSD: Makefile.playstation2.inc,v 1.8.6.2 2016/04/22 15:44:11 skrll Exp $

# Playstation2 uses 128 bit access, which currently requires n32 ABI
CFLAGS+= -mabi=n32 -march=r5900 -fno-pic -msoft-float
AFLAGS+= -mabi=n32 -march=r5900 -fno-pic -msoft-float
