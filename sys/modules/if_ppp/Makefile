# $NetBSD: Makefile,v 1.2.6.1 2019/06/10 22:09:28 christos Exp $

.include "../Makefile.inc"

.PATH:  ${S}/net

KMOD=		if_ppp
IOCONF=		ppp.ioconf
SRCS=		if_ppp.c ppp_tty.c

CPPFLAGS+=	-DINET -DINET6
CPPFLAGS+=	-DPPP_FILTER
CPPFLAGS+=	-DPPP_DEFLATE
CPPFLAGS+=	-DPPP_BSDCOMP

WARNS=		3

.include <bsd.kmodule.mk>
