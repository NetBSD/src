# $NetBSD: Makefile,v 1.1 2016/08/06 23:46:30 pgoyette Exp $

.include "../Makefile.inc"

.PATH:  ${S}/net

KMOD=		if_pppoe
IOCONF=		pppoe.ioconf
SRCS=		if_pppoe.c

CPPFLAGS+=	-DPPPOE_SERVER
#CPPFLAGS+=	-DPPPOE_TERM_UNKNOWN_SESSIONS	# This should not be
						# enabled by default

.include <bsd.kmodule.mk>
