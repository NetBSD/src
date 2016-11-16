# $NetBSD: Makefile,v 1.1 2016/11/16 21:36:23 kamil Exp $

NOMAN=		# defined

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/libpthread_dbg
LDADD+=		-lpthread
LDADD+=		-lpthread_dbg

TESTS_C+=	t_dummy

.include <bsd.test.mk>
