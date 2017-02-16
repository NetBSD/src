# $NetBSD: Makefile,v 1.1.6.2 2017/02/16 08:23:36 knakahara Exp $

.include "../Makefile.inc"

.PATH:  ${S}/net ${S}/netinet ${S}/netinet6

KMOD=		if_l2tp
IOCONF=		l2tp.ioconf
SRCS=		if_l2tp.c in_l2tp.c in6_l2tp.c

CPPFLAGS+=	-DINET
CPPFLAGS+=	-DINET6

.include <bsd.kmodule.mk>
