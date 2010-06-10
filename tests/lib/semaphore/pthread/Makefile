# $NetBSD: Makefile,v 1.1 2010/06/10 22:03:43 pooka Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/semaphore/pthread

TESTS_C=	t_sem_pth

CPPFLAGS+=	-I${.CURDIR}/..

LDADD+=		-lrumpkern_ksem -lrump -lrumpuser -lpthread

WARNS=	4

.include <bsd.test.mk>
