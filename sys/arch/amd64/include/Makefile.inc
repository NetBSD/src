# $NetBSD: Makefile.inc,v 1.3.24.1 2012/11/22 17:39:28 riz Exp $

CFLAGS+=	-mno-red-zone

# this should really be !(RUMPKERNEL && PIC)
.if !defined(RUMPKERNEL)
CFLAGS+=	-mcmodel=kernel -fno-omit-frame-pointer
.endif

USE_SSP?=	yes
