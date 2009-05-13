#	$NetBSD: Makefile,v 1.4.20.1 2009/05/13 19:20:32 jym Exp $

PROG=	powerd
SRCS=	powerd.c

DPADD+=	${LIBPROP} ${LIBUTIL}
LDADD+=	-lprop -lutil

MAN=	powerd.8

.include <bsd.prog.mk>
