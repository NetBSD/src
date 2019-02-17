# $NetBSD: Makefile,v 1.3 2019/02/17 04:05:52 rin Exp $

.include "../Makefile.inc"

.PATH:  ${S}/net

KMOD=		if_pppoe
IOCONF=		pppoe.ioconf
SRCS=		if_pppoe.c

CPPFLAGS+=	-DPPPOE_SERVER
CPPFLAGS+=	-DPPPOE_TERM_UNKNOWN_SESSIONS

WARNS=		3

.include <bsd.kmodule.mk>
