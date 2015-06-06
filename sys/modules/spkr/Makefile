# $NetBSD: Makefile,v 1.1.2.2 2015/06/06 14:40:24 skrll Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/isa

KMOD=	spkr
IOCONF=	spkr.ioconf
SRCS=	spkr.c

WARNS=	4

.include <bsd.kmodule.mk>
