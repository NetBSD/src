# $NetBSD: Makefile,v 1.2 2016/08/07 01:59:43 pgoyette Exp $

.include "../Makefile.inc"

.PATH:  ${S}/net

KMOD=		if_pppoe
IOCONF=		pppoe.ioconf
SRCS=		if_pppoe.c

CPPFLAGS+=	-DPPPOE_SERVER
CPPFLAGS+=	-DPPPOE_TERM_UNKNOWN_SESSIONS

.include <bsd.kmodule.mk>
