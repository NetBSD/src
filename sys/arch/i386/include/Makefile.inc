# $NetBSD: Makefile.inc,v 1.2 2014/05/24 18:39:04 jakllsch Exp $

# this should really be !(RUMPKERNEL && PIC)
.if !defined(RUMPKERNEL)
CFLAGS+=	-fno-omit-frame-pointer
.endif

USE_SSP?=	yes
