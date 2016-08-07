# $NetBSD: Makefile,v 1.1 2016/08/07 14:27:39 christos Exp $

.include "../Makefile.inc"

.PATH:  ${S}/net

KMOD=		if_mpls
IOCONF=		mpls.ioconf
SRCS=		if_mpls.c

CPPFLAGS+=	-DINET
CPPFLAGS+=	-DINET6

.include <bsd.kmodule.mk>
