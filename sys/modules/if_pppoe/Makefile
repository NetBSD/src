# $NetBSD: Makefile,v 1.2.18.2 2017/12/03 11:38:55 jdolecek Exp $

.include "../Makefile.inc"

.PATH:  ${S}/net

KMOD=		if_pppoe
IOCONF=		pppoe.ioconf
SRCS=		if_pppoe.c

CPPFLAGS+=	-DPPPOE_SERVER
CPPFLAGS+=	-DPPPOE_TERM_UNKNOWN_SESSIONS

.include <bsd.kmodule.mk>
