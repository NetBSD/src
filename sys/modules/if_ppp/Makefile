# $NetBSD: Makefile,v 1.2 2017/09/17 20:37:17 christos Exp $

.include "../Makefile.inc"

.PATH:  ${S}/net

KMOD=		if_ppp
IOCONF=		ppp.ioconf
SRCS=		if_ppp.c ppp_tty.c

CPPFLAGS+=	-DINET -DINET6
CPPFLAGS+=	-DPPP_FILTER
CPPFLAGS+=	-DPPP_DEFLATE
CPPFLAGS+=	-DPPP_BSDCOMP

.include <bsd.kmodule.mk>
