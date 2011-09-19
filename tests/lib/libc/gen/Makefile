# $NetBSD: Makefile,v 1.30 2011/09/19 05:40:38 jruoho Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/libc/gen

TESTS_C+=	t_alarm
TESTS_C+=	t_assert
TESTS_C+=	t_basedirname
TESTS_C+=	t_closefrom
TESTS_C+=	t_dir
TESTS_C+=	t_fmtcheck
TESTS_C+=	t_fpclassify
TESTS_C+=	t_fpsetmask
TESTS_C+=	t_fpsetround
TESTS_C+=	t_getcwd
TESTS_C+=	t_getgrent
TESTS_C+=	t_glob
TESTS_C+=	t_humanize_number
TESTS_C+=	t_isnan
TESTS_C+=	t_nice
TESTS_C+=	t_pause
TESTS_C+=	t_raise
TESTS_C+=	t_randomid
TESTS_C+=	t_setdomainname
TESTS_C+=	t_sethostname
TESTS_C+=	t_siginfo
TESTS_C+=	t_syslog
TESTS_C+=	t_ttyname
TESTS_C+=	t_vis

LDADD.t_fpclassify+=	-lm

LDADD.t_nice+=		-lpthread
LDADD.t_syslog+=	-lpthread

.if ${MACHINE_ARCH} == "alpha"
COPTS+=			-mieee
.endif

.include <bsd.test.mk>
