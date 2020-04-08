# $NetBSD: Makefile,v 1.3.16.1 2020/04/08 14:09:08 martin Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/kernel/tty

TESTS_C=	t_pr

CPPFLAGS+=	-D_KERNTYPES
LDADD+=		-lrumpkern_tty ${LIBRUMPBASE}

.include <bsd.test.mk>
