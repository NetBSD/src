# $NetBSD: Makefile,v 1.1 2015/05/17 05:21:38 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/isa

KMOD=	spkr
IOCONF=	spkr.ioconf
SRCS=	spkr.c

WARNS=	4

.include <bsd.kmodule.mk>
