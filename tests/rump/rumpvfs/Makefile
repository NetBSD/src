# $NetBSD: Makefile,v 1.3.10.1 2014/08/20 00:04:52 tls Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/rump/rumpvfs

TESTS_C=	t_basic
TESTS_C+=	t_etfs
TESTS_C+=	t_p2kifs

LDADD=	-lrumpvfs -lrump -lrumpuser -lrump -lpthread

WARNS=	4

.include <bsd.test.mk>
