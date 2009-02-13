#	$NetBSD: Makefile,v 1.5 2009/02/13 16:02:05 uebayasi Exp $

WARNS?=	4
PROG=	services_mkdb
MAN=	services_mkdb.8
SRCS=	services_mkdb.c uniq.c

LDADD+=	-lutil
DPADD+=	${LIBUTIL}
#COPTS+=-g

.include <bsd.prog.mk>
