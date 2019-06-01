# $NetBSD: Makefile,v 1.5 2019/06/01 06:59:17 kre Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/semaphore/pthread

TESTS_C=	t_sem_pth

CPPFLAGS+=	-I${.CURDIR}/.. -D_KERNTYPES

LDADD+=		-lrump -lrumpuser -lrump -lrumpvfs -lpthread

WARNS=	4

.include <bsd.test.mk>
