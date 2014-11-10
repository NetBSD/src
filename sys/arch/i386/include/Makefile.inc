# $NetBSD: Makefile.inc,v 1.1.22.1 2014/11/10 02:33:50 msaitoh Exp $

CFLAGS+=	-msoft-float
CFLAGS+=	-mno-mmx -mno-sse -mno-avx

USE_SSP?=	yes
