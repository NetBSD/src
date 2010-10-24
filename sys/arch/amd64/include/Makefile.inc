# $NetBSD: Makefile.inc,v 1.3.10.2 2010/10/24 22:47:52 jym Exp $

CFLAGS+=	-mno-red-zone

# this should really be !(RUMPKERNEL && PIC)
.if !defined(RUMPKERNEL)
CFLAGS+=	-mcmodel=kernel
.endif

USE_SSP?=	yes
