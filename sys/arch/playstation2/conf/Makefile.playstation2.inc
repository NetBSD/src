#	$NetBSD: Makefile.playstation2.inc,v 1.10 2016/03/28 10:31:45 martin Exp $

# Playstation2 uses 128 bit access, which currently requires n32 ABI
CFLAGS+= -mabi=n32 -march=r5900 -fno-pic -msoft-float
AFLAGS+= -mabi=n32 -march=r5900 -fno-pic -msoft-float
