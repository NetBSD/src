# $NetBSD: Makefile,v 1.13 2011/01/13 02:40:43 pgoyette Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/libc/gen

TESTS_C+=	t_basedirname
TESTS_C+=	t_dir
TESTS_C+=	t_fmtcheck
TESTS_C+=	t_glob_star
TESTS_C+=	t_humanize_number
TESTS_C+=	t_ldexp
TESTS_C+=	t_randomid
TESTS_C+=	t_rbstress
TESTS_C+=	t_siginfo
TESTS_C+=	t_syslog_pthread
TESTS_C+=	t_vis

LDADD.t_ldexp+=		-lm
LDADD.t_syslog_pthread+=-lpthread

.include <bsd.test.mk>
