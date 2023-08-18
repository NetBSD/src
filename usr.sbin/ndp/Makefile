# $NetBSD: Makefile,v 1.14 2023/08/18 13:07:38 tnn Exp $

.include <bsd.own.mk>

USE_FORT?=	yes	# network client

RUMPPRG=	ndp
SRCS=		ndp.c
MAN=		ndp.8

CPPFLAGS+=	-DINET6
CPPFLAGS+=	-I.

.if (${MKRUMP} != "no")
CPPFLAGS+= 	-DRUMP_ACTION
LDADD.rump+=	-lrumpres
DPADD.rump+=	${LIBRUMPRES}
.endif

.include <bsd.prog.mk>
