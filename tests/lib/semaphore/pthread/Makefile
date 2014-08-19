# $NetBSD: Makefile,v 1.2.2.1 2014/08/20 00:04:51 tls Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/semaphore/pthread

TESTS_C=	t_sem_pth

CPPFLAGS+=	-I${.CURDIR}/..

LDADD+=		-lrump -lrumpuser -lrump -lpthread

WARNS=	4

.include <bsd.test.mk>
