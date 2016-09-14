# $NetBSD: Makefile,v 1.2.2.2 2016/09/14 03:04:18 pgoyette Exp $

.include "../Makefile.inc"

.PATH:  ${S}/net

KMOD=		if_pppoe
IOCONF=		pppoe.ioconf
SRCS=		if_pppoe.c

CPPFLAGS+=	-DPPPOE_SERVER
CPPFLAGS+=	-DPPPOE_TERM_UNKNOWN_SESSIONS

.include <bsd.kmodule.mk>
