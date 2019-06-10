# $NetBSD: Makefile,v 1.3.4.1 2019/06/10 22:09:27 christos Exp $

.include "../Makefile.inc"

.PATH:  ${S}/net ${S}/netinet ${S}/netinet6

KMOD=		if_gif
IOCONF=		gif.ioconf
SRCS=		if_gif.c in_gif.c in6_gif.c

CPPFLAGS+=	-DINET
CPPFLAGS+=	-DINET6

WARNS=		3

.include <bsd.kmodule.mk>
