# $NetBSD: Makefile,v 1.4.24.1 2020/04/08 14:09:12 martin Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/rump/rumpvfs

TESTS_C=	t_basic
TESTS_C+=	t_etfs
TESTS_C+=	t_p2kifs

LDADD=	${LIBRUMPBASE}

WARNS=	4

.include <bsd.test.mk>
