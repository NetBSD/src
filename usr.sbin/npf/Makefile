# $NetBSD: Makefile,v 1.4 2012/09/13 21:02:50 martin Exp $

.include <bsd.own.mk>

SUBDIR=		npfctl

.if ${MKRUMP} != "no" && ${MKPIC} != "no"
SUBDIR+=	npftest
.endif

.include <bsd.subdir.mk>
