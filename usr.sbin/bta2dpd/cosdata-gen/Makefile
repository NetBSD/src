# $NetBSD: Makefile,v 1.2 2018/05/01 19:59:48 christos Exp $

WARNS?=		6

.include <bsd.hostinit.mk>

HOSTPROG=	cosdata
MAN=

DPADD+=		${LIBMATH}
LDADD+=		-lm

.include <bsd.hostprog.mk>
