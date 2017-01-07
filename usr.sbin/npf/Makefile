# $NetBSD: Makefile,v 1.6 2017/01/07 17:27:02 christos Exp $

.include <bsd.own.mk>

SUBDIR=		npfctl npfd

.if ${MKRUMP} != "no" && ${MKPIC} != "no"
SUBDIR+=	npftest
.endif

MAN=		npf.7

.include <bsd.man.mk>
.include <bsd.subdir.mk>
