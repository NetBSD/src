# $NetBSD: Makefile,v 1.2 2019/02/17 04:06:00 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/isa

KMOD=	wbsio
IOCONF=	wbsio.ioconf
SRCS=	wbsio.c

WARNS=	3

.include <bsd.kmodule.mk>
