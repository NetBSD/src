# $NetBSD: Makefile,v 1.2.12.1 2017/12/21 19:14:41 snj Exp $

.include "../Makefile.inc"

.PATH:  ${S}/net ${S}/netinet ${S}/netinet6

KMOD=		if_gif
IOCONF=		gif.ioconf
SRCS=		if_gif.c in_gif.c in6_gif.c

CPPFLAGS+=	-DINET
CPPFLAGS+=	-DINET6

.include <bsd.kmodule.mk>
