# $NetBSD: Makefile,v 1.1 2016/08/07 14:27:38 christos Exp $

.include "../Makefile.inc"

.PATH:  ${S}/net

KMOD=		if_gif
IOCONF=		gif.ioconf
SRCS=		if_gif.c

CPPFLAGS+=	-DINET
CPPFLAGS+=	-DINET6

.include <bsd.kmodule.mk>
