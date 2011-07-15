# $NetBSD: Makefile,v 1.1 2011/07/15 20:57:01 jmcneill Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/isa

KMOD=	nsclpcsio
IOCONF=	nsclpcsio.ioconf
SRCS=	nsclpcsio_isa.c

WARNS=	3

.include <bsd.kmodule.mk>
