# $NetBSD: Makefile,v 1.1 2010/06/16 19:29:33 pooka Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/rump/rumpvfs

TESTS_C=	t_etfs

LDADD=	-lrumpvfs -lrump -lrumpuser -lpthread

WARNS=	4

.include <bsd.test.mk>
