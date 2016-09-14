# $NetBSD: Makefile,v 1.1.2.2 2016/09/14 03:04:18 pgoyette Exp $

.include "../Makefile.inc"

.PATH:  ${S}/net

KMOD=		if_strip
IOCONF=		strip.ioconf
SRCS=		if_strip.c

CPPFLAGS+=	-DINET

.include <bsd.kmodule.mk>
