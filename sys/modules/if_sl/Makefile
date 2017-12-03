# $NetBSD: Makefile,v 1.1.18.2 2017/12/03 11:38:55 jdolecek Exp $

.include "../Makefile.inc"

.PATH:  ${S}/net

KMOD=		if_sl
IOCONF=		sl.ioconf
SRCS=		if_sl.c

CPPFLAGS+=	-DINET

.include <bsd.kmodule.mk>
