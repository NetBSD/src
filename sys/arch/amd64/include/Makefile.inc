# $NetBSD: Makefile.inc,v 1.3 2009/11/27 13:50:29 pooka Exp $

CFLAGS+=	-mno-red-zone

# this should really be !(RUMPKERNEL && PIC)
.if !defined(RUMPKERNEL)
CFLAGS+=	-mcmodel=kernel
.endif

USE_SSP?=	yes
