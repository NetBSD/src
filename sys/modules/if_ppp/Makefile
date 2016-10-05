# $NetBSD: Makefile,v 1.1.4.2 2016/10/05 20:56:05 skrll Exp $

.include "../Makefile.inc"

.PATH:  ${S}/net

KMOD=		if_ppp
IOCONF=		ppp.ioconf
SRCS=		if_ppp.c ppp_tty.c

CPPFLAGS+=	-DINET
CPPFLAGS+=	-DPPP_FILTER
CPPFLAGS+=	-DPPP_DEFLATE
CPPFLAGS+=	-DPPP_BSDCOMP

.include <bsd.kmodule.mk>
