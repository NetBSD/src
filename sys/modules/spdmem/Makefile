# $NetBSD: Makefile,v 1.4 2019/02/17 04:05:58 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/i2c

KMOD=	spdmem
IOCONF=	spdmem.ioconf
SRCS=	spdmem_i2c.c

.PATH:	${S}/dev/ic

SRCS+=	spdmem.c

.include <bsd.kmodule.mk>
