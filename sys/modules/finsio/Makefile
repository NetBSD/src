# $NetBSD: Makefile,v 1.1 2011/07/31 17:54:01 jmcneill Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/isa

KMOD=	finsio
IOCONF=	finsio.ioconf
SRCS=	finsio_isa.c

WARNS=	4

.include <bsd.kmodule.mk>
