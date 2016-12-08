# $NetBSD: Makefile,v 1.3 2016/12/08 23:22:18 nat Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/isa

KMOD=	spkr
IOCONF=	spkr.ioconf
CFLAGS+= -DPCPPISPEAKER
SRCS=	spkr.c

WARNS=	4

.include <bsd.kmodule.mk>
