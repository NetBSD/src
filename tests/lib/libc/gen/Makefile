# $NetBSD: Makefile,v 1.10 2010/12/28 20:25:31 pgoyette Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/libc/gen

TESTS_C+=	t_basedirname
TESTS_C+=	t_dir
TESTS_C+=	t_fmtcheck
TESTS_C+=	t_glob_star
TESTS_C+=	t_humanize_number
TESTS_C+=	t_rbstress
TESTS_C+=	t_syslog_pthread
TESTS_C+=	t_vis

.if !make(obj) && !make(cleandir)
HAVE_SIGINFO != if (echo "\#include <signal.h>" && echo "SA_SIGINFO") | ${CC} -E -I${DESTDIR}/usr/include - | grep -sq 0x0040; then echo yes; else echo no; fi
.endif

.if make(obj) || make(cleandir) || (${HAVE_SIGINFO} == "yes")
TESTS_C+=	t_siginfo
.endif

.if (${MACHINE_ARCH} != "vax")
TESTS_C+=	t_ldexp
.endif

LDADD.t_syslog_pthread+=-lpthread

.include <bsd.test.mk>
