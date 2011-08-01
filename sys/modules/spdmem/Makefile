# $NetBSD: Makefile,v 1.1 2011/08/01 03:51:05 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/i2c

KMOD=	spdmem
IOCONF=	spdmem.ioconf
SRCS=	spdmem_i2c.c

.PATH:	${S}/dev/ic

SRCS+=	spdmem.c

WARNS=	4

.include <bsd.kmodule.mk>
