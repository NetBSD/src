# $NetBSD: Makefile,v 1.4.16.1 2019/06/10 22:10:08 christos Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/semaphore/pthread

TESTS_C=	t_sem_pth

CPPFLAGS+=	-I${.CURDIR}/.. -D_KERNTYPES

LDADD+=		-lrump -lrumpuser -lrump -lrumpvfs -lpthread

WARNS=	4

.include <bsd.test.mk>
