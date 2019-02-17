# $NetBSD: Makefile,v 1.3 2019/02/17 04:05:55 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/isa

KMOD=	nsclpcsio
IOCONF=	nsclpcsio.ioconf
SRCS=	nsclpcsio_isa.c

WARNS=	3

.include <bsd.kmodule.mk>
