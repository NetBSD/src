#	$NetBSD: Makefile,v 1.4.18.1 2010/12/08 04:31:41 riz Exp $

WARNS?=	4
PROG=	services_mkdb
MAN=	services_mkdb.8
SRCS=	services_mkdb.c uniq.c

LDADD+=	-lutil
DPADD+=	${LIBUTIL}
#COPTS+=-g

.include <bsd.prog.mk>
