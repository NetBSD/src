# $NetBSD: Makefile,v 1.1 2016/08/06 12:44:03 christos Exp $

.include "../Makefile.inc"

.PATH:  ${S}/net

KMOD=		if_sl
IOCONF=		sl.ioconf
SRCS=		if_sl.c

CPPFLAGS+=	-DINET

.include <bsd.kmodule.mk>
