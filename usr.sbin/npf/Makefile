# $NetBSD: Makefile,v 1.5.6.1 2017/03/20 06:58:08 pgoyette Exp $

.include <bsd.own.mk>

SUBDIR=		npfctl npfd

.if ${MKRUMP} != "no" && ${MKPIC} != "no"
SUBDIR+=	npftest
.endif

MAN=		npf.7

.include <bsd.man.mk>
.include <bsd.subdir.mk>
