# $NetBSD: Makefile.inc,v 1.4 2026/10/08 19:11:09 riastradh Exp $

CFLAGS+=	-msoft-float
CFLAGS+=	-mno-mmx -mno-sse -mno-avx

# this should really be !(RUMPKERNEL && PIC)
.if !defined(RUMPKERNEL)
CFLAGS+=	-fno-omit-frame-pointer
.endif

USE_SSP_DEFAULT=yes
