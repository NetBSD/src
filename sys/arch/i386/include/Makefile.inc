# $NetBSD: Makefile.inc,v 1.1.26.2 2017/12/03 11:36:17 jdolecek Exp $

CFLAGS+=	-msoft-float
CFLAGS+=	-mno-mmx -mno-sse -mno-avx

# this should really be !(RUMPKERNEL && PIC)
.if !defined(RUMPKERNEL)
CFLAGS+=	-fno-omit-frame-pointer
.endif

USE_SSP?=	yes
