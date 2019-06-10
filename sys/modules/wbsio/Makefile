# $NetBSD: Makefile,v 1.1.50.1 2019/06/10 22:09:43 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/isa

KMOD=	wbsio
IOCONF=	wbsio.ioconf
SRCS=	wbsio.c

WARNS=	3

.include <bsd.kmodule.mk>
