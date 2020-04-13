# $NetBSD: Makefile,v 1.6.14.1 2020/04/13 08:05:55 martin Exp $

.include <bsd.own.mk>

SUBDIR=		npfctl npfd

.if ${MKRUMP} != "no" && ${MKPIC} != "no"
SUBDIR+=	npftest
.endif

MAN=		npf.7 npf-params.7

.include <bsd.man.mk>
.include <bsd.subdir.mk>
