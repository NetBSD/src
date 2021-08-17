# $NetBSD: Makefile,v 1.1 2021/08/17 17:29:20 christos Exp $

.include "../Makefile.inc"

.PATH:  ${S}/net

KMOD=		if_wg
IOCONF=		wg.ioconf
SRCS=		if_wg.c

CPPFLAGS+=	-DINET
CPPFLAGS+=	-DINET6
CPPFLAGS+=	-DALTQ

WARNS=		3

.include <bsd.kmodule.mk>
