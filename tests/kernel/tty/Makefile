# $NetBSD: Makefile,v 1.4 2020/03/01 18:08:15 christos Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/kernel/tty

TESTS_C=	t_pr

CPPFLAGS+=	-D_KERNTYPES
LDADD+=		-lrumpkern_tty ${LIBRUMPBASE}

.include <bsd.test.mk>
