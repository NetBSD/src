#	$NetBSD: Makefile,v 1.3 2006/02/05 14:11:18 christos Exp $

PROG=	powerd
SRCS=	powerd.c

WARNS?= 4
DPADD+=	${LIBUTIL}
LDADD+=	-lutil

MAN=	powerd.8

.include <bsd.prog.mk>
