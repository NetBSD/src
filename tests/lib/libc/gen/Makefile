# $NetBSD: Makefile,v 1.17 2011/04/05 06:15:31 jruoho Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/libc/gen

TESTS_C+=	t_basedirname
TESTS_C+=	t_dir
TESTS_C+=	t_fmtcheck
TESTS_C+=	t_getgrent
TESTS_C+=	t_glob_star
TESTS_C+=	t_humanize_number
TESTS_C+=	t_ldexp
TESTS_C+=	t_nice
TESTS_C+=	t_raise
TESTS_C+=	t_randomid
TESTS_C+=	t_rbstress
TESTS_C+=	t_siginfo
TESTS_C+=	t_syslog_pthread
TESTS_C+=	t_ttyname
TESTS_C+=	t_vis

LDADD.t_ldexp+=		-lm
LDADD.t_syslog_pthread+=-lpthread

.include <bsd.test.mk>
