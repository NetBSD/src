# $NetBSD: Makefile.inc,v 1.3.56.1 2026/11/04 23:20:44 perseant Exp $

CFLAGS+=	-msoft-float
CFLAGS+=	-mno-mmx -mno-sse -mno-avx

# this should really be !(RUMPKERNEL && PIC)
.if !defined(RUMPKERNEL)
CFLAGS+=	-fno-omit-frame-pointer
.endif

USE_SSP_DEFAULT=yes
