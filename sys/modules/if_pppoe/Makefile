# $NetBSD: Makefile,v 1.2.22.1 2019/06/10 22:09:28 christos Exp $

.include "../Makefile.inc"

.PATH:  ${S}/net

KMOD=		if_pppoe
IOCONF=		pppoe.ioconf
SRCS=		if_pppoe.c

CPPFLAGS+=	-DPPPOE_SERVER
CPPFLAGS+=	-DPPPOE_TERM_UNKNOWN_SESSIONS

WARNS=		3

.include <bsd.kmodule.mk>
