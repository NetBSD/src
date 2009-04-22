#	$NetBSD: Makefile,v 1.6 2009/04/22 15:23:08 lukem Exp $

PROG=	services_mkdb
MAN=	services_mkdb.8
SRCS=	services_mkdb.c uniq.c

LDADD+=	-lutil
DPADD+=	${LIBUTIL}
#COPTS+=-g

.include <bsd.prog.mk>
