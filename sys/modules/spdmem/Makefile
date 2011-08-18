# $NetBSD: Makefile,v 1.2 2011/08/18 17:02:49 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/i2c

KMOD=	spdmem
IOCONF=	spdmem.ioconf
SRCS=	spdmem_i2c.c

.PATH:	${S}/dev/ic

SRCS+=	spdmem.c
COPTS.spdmem.c += -Wno-format-nonliteral

WARNS=	4

.include <bsd.kmodule.mk>
