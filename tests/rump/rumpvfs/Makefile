# $NetBSD: Makefile,v 1.2.2.1 2011/03/05 15:10:57 bouyer Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/rump/rumpvfs

TESTS_C=	t_basic
TESTS_C+=	t_etfs
TESTS_C+=	t_p2kifs

LDADD=	-lrumpvfs -lrump -lrumpuser -lpthread

WARNS=	4

.include <bsd.test.mk>
