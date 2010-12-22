# $NetBSD: Makefile,v 1.3 2010/12/22 23:45:44 pgoyette Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/libc/gen

TESTS_C+=	t_glob_star
TESTS_C+=	t_syslog_pthread
TESTS_C+=	t_basedirname

LDADD.t_syslog_pthread+=-lpthread

.include <bsd.test.mk>
