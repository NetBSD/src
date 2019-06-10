# $NetBSD: Makefile,v 1.2.54.1 2019/06/10 22:09:35 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/isa

KMOD=	nsclpcsio
IOCONF=	nsclpcsio.ioconf
SRCS=	nsclpcsio_isa.c

WARNS=	3

.include <bsd.kmodule.mk>
