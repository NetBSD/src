# $NetBSD: Makefile,v 1.8 2021/11/23 23:29:55 thorpej Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/libexecinfo

TESTS_C+=	t_backtrace
TESTS_C+=	t_sig_backtrace
STRIPFLAG=

LDADD+=		-lexecinfo -lelf
DPADD+=		${LIBEXECINFO} ${LIBELF}

COPTS.t_backtrace.c=	-Wno-stack-protector

# XXX temporary hack
.if ${MACHINE_CPU} == "arm"
COPTS.t_sig_backtrace.c=-fno-omit-frame-pointer
CPPFLAGS.t_sig_backtrace.c=-DNOINLINE_HACK
.endif

.include <bsd.test.mk>
