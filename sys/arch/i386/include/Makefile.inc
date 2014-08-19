# $NetBSD: Makefile.inc,v 1.1.26.1 2014/08/20 00:03:06 tls Exp $

# this should really be !(RUMPKERNEL && PIC)
.if !defined(RUMPKERNEL)
CFLAGS+=	-fno-omit-frame-pointer
.endif

USE_SSP?=	yes
