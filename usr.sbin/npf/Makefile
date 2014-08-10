# $NetBSD: Makefile,v 1.4.6.1 2014/08/10 07:00:01 tls Exp $

.include <bsd.own.mk>

SUBDIR=		npfctl

.if ${MKRUMP} != "no" && ${MKPIC} != "no"
SUBDIR+=	npftest
.endif

MAN=		npf.7

.include <bsd.man.mk>
.include <bsd.subdir.mk>
