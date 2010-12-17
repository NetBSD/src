# $NetBSD: Makefile,v 1.2 2010/12/17 19:12:30 pooka Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/libc/gen

TESTS_C+=	t_glob_star
TESTS_C+=	t_syslog_pthread

LDADD.t_syslog_pthread+=-lpthread

.include <bsd.test.mk>
