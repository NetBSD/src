# $NetBSD: Makefile,v 1.1 2011/07/29 20:59:25 jmcneill Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/isa

KMOD=	itesio
IOCONF=	itesio.ioconf
SRCS=	itesio_isa.c

WARNS=	5

.include <bsd.kmodule.mk>
