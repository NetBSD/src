# $NetBSD: Makefile,v 1.1 2010/07/18 12:41:51 jmmv Exp $

TESTSDIR=	${TESTSBASE}/lib/libobjc

TESTS_C=	t_threads
SRCS.t_threads=	t_threads.m
WARNS?=		4
CFLAGS+=	-pthread
LDFLAGS+=	-pthread
DPADD+=		${LIBOBJC}
LDADD+=		-lobjc

.include <bsd.test.mk>
