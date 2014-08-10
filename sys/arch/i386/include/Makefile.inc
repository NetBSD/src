# $NetBSD: Makefile.inc,v 1.1.40.1 2014/08/10 06:53:59 tls Exp $

# this should really be !(RUMPKERNEL && PIC)
.if !defined(RUMPKERNEL)
CFLAGS+=	-fno-omit-frame-pointer
.endif

USE_SSP?=	yes
