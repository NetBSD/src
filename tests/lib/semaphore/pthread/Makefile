# $NetBSD: Makefile,v 1.6 2020/03/01 18:08:15 christos Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/semaphore/pthread

TESTS_C=	t_sem_pth

CPPFLAGS+=	-I${.CURDIR}/.. -D_KERNTYPES

LDADD+=		${LIBRUMPBASE}

WARNS=	4

.include <bsd.test.mk>
