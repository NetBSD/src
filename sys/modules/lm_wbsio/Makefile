# $NetBSD: Makefile,v 1.1 2012/01/17 16:50:07 jakllsch Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/isa

KMOD=	lm_wbsio
IOCONF=	lm_wbsio.ioconf
SRCS=	lm_wbsio.c

WARNS?=	4

.include <bsd.kmodule.mk>
