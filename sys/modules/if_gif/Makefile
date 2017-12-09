# $NetBSD: Makefile,v 1.3 2017/12/09 08:03:07 pgoyette Exp $

.include "../Makefile.inc"

.PATH:  ${S}/net ${S}/netinet ${S}/netinet6

KMOD=		if_gif
IOCONF=		gif.ioconf
SRCS=		if_gif.c in_gif.c in6_gif.c

CPPFLAGS+=	-DINET
CPPFLAGS+=	-DINET6

.include <bsd.kmodule.mk>
