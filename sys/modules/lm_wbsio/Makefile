# $NetBSD: Makefile,v 1.1.4.2 2012/04/17 00:08:37 yamt Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/isa

KMOD=	lm_wbsio
IOCONF=	lm_wbsio.ioconf
SRCS=	lm_wbsio.c

WARNS?=	4

.include <bsd.kmodule.mk>
