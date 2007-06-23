#	$NetBSD: Makefile,v 1.4 2007/06/23 16:55:15 christos Exp $

WARNS?=	4
PROG=	services_mkdb
MAN=	services_mkdb.8
SRCS=	services_mkdb.c uniq.c

LDADD+=	-lutil
DPADD+=	${LIBUTIL}
COPTS+=-g

.include <bsd.prog.mk>
