# $NetBSD: Makefile,v 1.1.2.2 2016/09/14 03:04:17 pgoyette Exp $

.include "../Makefile.inc"

.PATH:  ${S}/net

KMOD=		if_faith
IOCONF=		faith.ioconf
SRCS=		if_faith.c

CPPFLAGS+=	-DINET
CPPFLAGS+=	-DINET6

.include <bsd.kmodule.mk>
