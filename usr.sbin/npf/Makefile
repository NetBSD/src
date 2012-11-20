# $NetBSD: Makefile,v 1.1.12.1 2012/11/20 03:03:03 tls Exp $

.include <bsd.own.mk>

SUBDIR=		npfctl

.if ${MKRUMP} != "no" && ${MKPIC} != "no"
SUBDIR+=	npftest
.endif

.include <bsd.subdir.mk>
