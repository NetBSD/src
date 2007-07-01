#	$NetBSD: Makefile,v 1.4 2007/07/01 07:39:47 xtraeme Exp $

PROG=	powerd
SRCS=	powerd.c

WARNS?= 4
DPADD+=	${LIBPROP} ${LIBUTIL}
LDADD+=	-lprop -lutil

MAN=	powerd.8

.include <bsd.prog.mk>
