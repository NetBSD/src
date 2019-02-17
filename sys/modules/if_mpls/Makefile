# $NetBSD: Makefile,v 1.2 2019/02/17 04:05:51 rin Exp $

.include "../Makefile.inc"

.PATH:  ${S}/net

KMOD=		if_mpls
IOCONF=		mpls.ioconf
SRCS=		if_mpls.c

CPPFLAGS+=	-DINET
CPPFLAGS+=	-DINET6

WARNS=		3

.include <bsd.kmodule.mk>
