# $NetBSD: Makefile,v 1.7 2019/07/25 00:02:55 rmind Exp $

.include <bsd.own.mk>

SUBDIR=		npfctl npfd

.if ${MKRUMP} != "no" && ${MKPIC} != "no"
SUBDIR+=	npftest
.endif

MAN=		npf.7 npf-params.7

.include <bsd.man.mk>
.include <bsd.subdir.mk>
