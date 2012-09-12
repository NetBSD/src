# $NetBSD: Makefile,v 1.3 2012/09/12 19:20:37 rjs Exp $

.include <bsd.own.mk>

SUBDIR=		npfctl

.if (${MKRUMP} != "no")
SUBDIR+=	npftest
.endif

.include <bsd.subdir.mk>
