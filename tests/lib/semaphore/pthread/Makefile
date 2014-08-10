# $NetBSD: Makefile,v 1.2.8.1 2014/08/10 06:57:22 tls Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/semaphore/pthread

TESTS_C=	t_sem_pth

CPPFLAGS+=	-I${.CURDIR}/..

LDADD+=		-lrump -lrumpuser -lrump -lpthread

WARNS=	4

.include <bsd.test.mk>
