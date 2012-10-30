# $NetBSD: Makefile,v 1.1.6.1 2012/10/30 19:00:42 yamt Exp $

.include <bsd.own.mk>

SUBDIR=		npfctl

.if ${MKRUMP} != "no" && ${MKPIC} != "no"
SUBDIR+=	npftest
.endif

.include <bsd.subdir.mk>
