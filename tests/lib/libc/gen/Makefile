# $NetBSD: Makefile,v 1.5 2010/12/25 01:22:02 pgoyette Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/libc/gen

TESTS_C+=	t_glob_star
TESTS_C+=	t_syslog_pthread
TESTS_C+=	t_basedirname
TESTS_C+=	t_siginfo

.if !make(obj) && !make(cleandir)
HAVE_SIGINFO != if (echo "\#include <signal.h>" && echo "SA_SIGINFO") | ${CC} -E -I${DESTDIR}/usr/include - | grep -sq 0x0040; then echo yes; else echo no; fi
.endif

.if make(obj) || make(cleandir) || (${HAVE_SIGINFO} == "yes")
TESTS_C+=	t_siginfo
.endif

LDADD.t_syslog_pthread+=-lpthread

.include <bsd.test.mk>
