# $NetBSD: Makefile.inc,v 1.5.58.1 2026/11/17 15:54:43 martin Exp $

CFLAGS+=	-mno-red-zone -mno-mmx -mno-sse -mno-avx
CFLAGS+=	-msoft-float

# this should really be !(RUMPKERNEL && PIC)
.if !defined(RUMPKERNEL)
CFLAGS+=	-mcmodel=kernel -fno-omit-frame-pointer
.endif

USE_SSP_DEFAULT=yes
