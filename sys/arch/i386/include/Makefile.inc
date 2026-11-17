# $NetBSD: Makefile.inc,v 1.3.50.1 2026/11/17 16:07:09 martin Exp $

CFLAGS+=	-msoft-float
CFLAGS+=	-mno-mmx -mno-sse -mno-avx

# this should really be !(RUMPKERNEL && PIC)
.if !defined(RUMPKERNEL)
CFLAGS+=	-fno-omit-frame-pointer
.endif

USE_SSP_DEFAULT=yes
