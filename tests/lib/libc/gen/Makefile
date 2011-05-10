# $NetBSD: Makefile,v 1.23 2011/05/10 13:03:06 jruoho Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/libc/gen

TESTS_C+=	t_alarm
TESTS_C+=	t_basedirname
TESTS_C+=	t_closefrom
TESTS_C+=	t_dir
TESTS_C+=	t_fmtcheck
TESTS_C+=	t_getcwd
TESTS_C+=	t_getgrent
TESTS_C+=	t_glob_star
TESTS_C+=	t_humanize_number
TESTS_C+=	t_ldexp
TESTS_C+=	t_nice
TESTS_C+=	t_pause
TESTS_C+=	t_raise
TESTS_C+=	t_randomid
TESTS_C+=	t_siginfo
TESTS_C+=	t_syslog_pthread
TESTS_C+=	t_ttyname
TESTS_C+=	t_vis

LDADD.t_ldexp+=		-lm
LDADD.t_nice+=		-lpthread
LDADD.t_syslog_pthread+=-lpthread

.include <bsd.test.mk>
