# $NetBSD: Makefile.inc,v 1.3.4.2 2010/03/11 15:01:59 yamt Exp $

CFLAGS+=	-mno-red-zone

# this should really be !(RUMPKERNEL && PIC)
.if !defined(RUMPKERNEL)
CFLAGS+=	-mcmodel=kernel
.endif

USE_SSP?=	yes
