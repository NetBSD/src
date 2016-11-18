# $NetBSD: Makefile,v 1.2 2016/11/18 22:50:19 kamil Exp $

NOMAN=		# defined

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/libpthread_dbg
LDADD+=		-lpthread
LDADD+=		-lpthread_dbg

TESTS_C+=	t_dummy
TESTS_C+=	t_threads

.include <bsd.test.mk>
