# $NetBSD: Makefile,v 1.5 2020/03/01 18:08:16 christos Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/rump/rumpvfs

TESTS_C=	t_basic
TESTS_C+=	t_etfs
TESTS_C+=	t_p2kifs

LDADD=	${LIBRUMPBASE}

WARNS=	4

.include <bsd.test.mk>
