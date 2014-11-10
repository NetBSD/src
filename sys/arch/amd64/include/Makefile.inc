# $NetBSD: Makefile.inc,v 1.3.24.2 2014/11/10 02:33:50 msaitoh Exp $

CFLAGS+=	-mno-red-zone -mno-mmx -mno-sse -mno-avx
CFLAGS+=	-msoft-float

# this should really be !(RUMPKERNEL && PIC)
.if !defined(RUMPKERNEL)
CFLAGS+=	-mcmodel=kernel -fno-omit-frame-pointer
.endif

USE_SSP?=	yes
