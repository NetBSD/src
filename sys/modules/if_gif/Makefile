# $NetBSD: Makefile,v 1.2.2.2 2016/09/14 03:04:18 pgoyette Exp $

.include "../Makefile.inc"

.PATH:  ${S}/net ${S}/netinet ${S}/netinet6

KMOD=		if_gif
IOCONF=		gif.ioconf
SRCS=		if_gif.c in_gif.c in6_gif.c ip_ecn.c

CPPFLAGS+=	-DINET
CPPFLAGS+=	-DINET6

.include <bsd.kmodule.mk>
