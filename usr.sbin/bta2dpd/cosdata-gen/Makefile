# $NetBSD: Makefile,v 1.1.14.1 2018/05/02 07:20:28 pgoyette Exp $

WARNS?=		6

.include <bsd.hostinit.mk>

HOSTPROG=	cosdata
MAN=

DPADD+=		${LIBMATH}
LDADD+=		-lm

.include <bsd.hostprog.mk>
