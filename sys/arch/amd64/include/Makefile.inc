# $NetBSD: Makefile.inc,v 1.4.14.1 2014/10/19 16:11:06 martin Exp $

CFLAGS+=	-mno-red-zone -mno-mmx -mno-sse -mno-avx
CFLAGS+=	-msoft-float

# this should really be !(RUMPKERNEL && PIC)
.if !defined(RUMPKERNEL)
CFLAGS+=	-mcmodel=kernel -fno-omit-frame-pointer
.endif

USE_SSP?=	yes
