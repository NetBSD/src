# $NetBSD: Makefile,v 1.1.2.2 2016/09/14 03:04:18 pgoyette Exp $

.include "../Makefile.inc"

.PATH:  ${S}/net

KMOD=		if_mpls
IOCONF=		mpls.ioconf
SRCS=		if_mpls.c

CPPFLAGS+=	-DINET
CPPFLAGS+=	-DINET6

.include <bsd.kmodule.mk>
