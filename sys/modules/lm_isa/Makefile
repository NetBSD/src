# $NetBSD: Makefile,v 1.1 2012/01/17 16:50:07 jakllsch Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/isa

KMOD=	lm_isa
IOCONF=	lm_isa.ioconf
SRCS=	lm_isa.c

WARNS?=	4

.include <bsd.kmodule.mk>
