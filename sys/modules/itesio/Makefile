# $NetBSD: Makefile,v 1.2 2011/10/13 17:23:29 plunky Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/isa

KMOD=	itesio
IOCONF=	itesio.ioconf
SRCS=	itesio_isa.c

WARNS?=	4

.include <bsd.kmodule.mk>
