# $NetBSD: Makefile,v 1.1.22.1 2019/06/10 22:09:28 christos Exp $

.include "../Makefile.inc"

.PATH:  ${S}/net

KMOD=		if_mpls
IOCONF=		mpls.ioconf
SRCS=		if_mpls.c

CPPFLAGS+=	-DINET
CPPFLAGS+=	-DINET6

WARNS=		3

.include <bsd.kmodule.mk>
