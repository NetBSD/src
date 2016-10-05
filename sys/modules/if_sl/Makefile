# $NetBSD: Makefile,v 1.1.4.2 2016/10/05 20:56:05 skrll Exp $

.include "../Makefile.inc"

.PATH:  ${S}/net

KMOD=		if_sl
IOCONF=		sl.ioconf
SRCS=		if_sl.c

CPPFLAGS+=	-DINET

.include <bsd.kmodule.mk>
