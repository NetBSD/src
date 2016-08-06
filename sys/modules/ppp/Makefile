# $NetBSD: Makefile,v 1.1 2016/08/06 02:35:05 pgoyette Exp $

.include "../Makefile.inc"

.PATH:  ${S}/net

KMOD=		ppp
IOCONF=		ppp.ioconf
SRCS=		if_ppp.c ppp_tty.c

CPPFLAGS+=	-DINET
CPPFLAGS+=	-DPPP_FILTER
CPPFLAGS+=	-DPPP_DEFLATE
CPPFLAGS+=	-DPPP_BSDCOMP

.include <bsd.kmodule.mk>
