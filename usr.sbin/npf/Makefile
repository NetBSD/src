# $NetBSD: Makefile,v 1.1.12.2 2014/08/20 00:05:11 tls Exp $

.include <bsd.own.mk>

SUBDIR=		npfctl

.if ${MKRUMP} != "no" && ${MKPIC} != "no"
SUBDIR+=	npftest
.endif

MAN=		npf.7

.include <bsd.man.mk>
.include <bsd.subdir.mk>
