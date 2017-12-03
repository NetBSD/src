# $NetBSD: Makefile,v 1.1.18.2 2017/12/03 11:38:55 jdolecek Exp $

.include "../Makefile.inc"

.PATH:  ${S}/net

KMOD=		if_faith
IOCONF=		faith.ioconf
SRCS=		if_faith.c

CPPFLAGS+=	-DINET
CPPFLAGS+=	-DINET6

.include <bsd.kmodule.mk>
