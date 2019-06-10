# $NetBSD: Makefile,v 1.3.54.1 2019/06/10 22:09:39 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/i2c

KMOD=	spdmem
IOCONF=	spdmem.ioconf
SRCS=	spdmem_i2c.c

.PATH:	${S}/dev/ic

SRCS+=	spdmem.c

.include <bsd.kmodule.mk>
