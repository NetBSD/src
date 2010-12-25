# $NetBSD: Makefile,v 1.4 2010/12/25 01:04:26 pgoyette Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/libc/gen

TESTS_C+=	t_glob_star
TESTS_C+=	t_syslog_pthread
TESTS_C+=	t_basedirname
TESTS_C+=	t_siginfo

LDADD.t_syslog_pthread+=-lpthread

.include <bsd.test.mk>
