#	$NetBSD: Makefile.playstation2.inc,v 1.8.4.3 2017/12/03 11:36:34 jdolecek Exp $

# Playstation2 uses 128 bit access, which currently requires n32 ABI
CFLAGS+= -mabi=n32 -march=r5900 -fno-pic -msoft-float
AFLAGS+= -mabi=n32 -march=r5900 -fno-pic -msoft-float
