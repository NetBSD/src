# $NetBSD: Makefile,v 1.1 2019/03/24 11:20:26 pgoyette Exp $

.include "../Makefile.inc"

.PATH:  ${S}/net

KMOD=		if_tap
IOCONF=		tap.ioconf
SRCS=		if_tap.c

CPPFLAGS+=	-DINET
CPPFLAGS+=	-DINET6

.include <bsd.kmodule.mk>
