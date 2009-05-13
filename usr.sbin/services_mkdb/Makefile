#	$NetBSD: Makefile,v 1.4.20.1 2009/05/13 19:20:40 jym Exp $

PROG=	services_mkdb
MAN=	services_mkdb.8
SRCS=	services_mkdb.c uniq.c

LDADD+=	-lutil
DPADD+=	${LIBUTIL}
#COPTS+=-g

.include <bsd.prog.mk>
