# $NetBSD: Makefile,v 1.3 2019/02/17 04:05:51 rin Exp $

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
