# $NetBSD: Makefile,v 1.4.16.2 2020/04/08 14:09:10 martin Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/semaphore/pthread

TESTS_C=	t_sem_pth

CPPFLAGS+=	-I${.CURDIR}/.. -D_KERNTYPES

LDADD+=		${LIBRUMPBASE}

WARNS=	4

.include <bsd.test.mk>
