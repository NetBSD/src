# $NetBSD: Makefile,v 1.2.2.2 2017/01/07 08:56:55 pgoyette Exp $

NOMAN=		# defined

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/libpthread_dbg
LDADD+=		-lpthread
LDADD+=		-lpthread_dbg

TESTS_C+=	t_dummy
TESTS_C+=	t_threads

.include <bsd.test.mk>
