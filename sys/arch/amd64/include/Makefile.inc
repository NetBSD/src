# $NetBSD: Makefile.inc,v 1.3.18.1 2012/10/30 17:18:45 yamt Exp $

CFLAGS+=	-mno-red-zone

# this should really be !(RUMPKERNEL && PIC)
.if !defined(RUMPKERNEL)
CFLAGS+=	-mcmodel=kernel -fno-omit-frame-pointer
.endif

USE_SSP?=	yes
