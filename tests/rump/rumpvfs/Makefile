# $NetBSD: Makefile,v 1.2 2010/11/30 16:16:33 pooka Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/rump/rumpvfs

TESTS_C=	t_etfs
TESTS_C+=	t_p2kifs

LDADD=	-lrumpvfs -lrump -lrumpuser -lpthread

WARNS=	4

.include <bsd.test.mk>
