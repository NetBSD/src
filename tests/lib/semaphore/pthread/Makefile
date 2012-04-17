# $NetBSD: Makefile,v 1.1.6.1 2012/04/17 00:09:14 yamt Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/semaphore/pthread

TESTS_C=	t_sem_pth

CPPFLAGS+=	-I${.CURDIR}/..

LDADD+=		-lrump -lrumpuser -lpthread

WARNS=	4

.include <bsd.test.mk>
