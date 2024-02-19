# $NetBSD: Makefile,v 1.3 2024/02/19 04:30:39 riastradh Exp $

NOMAN=		# defined

.include <bsd.own.mk>

WARNS=	4

TESTSDIR=	${TESTSBASE}/lib/libc/setjmp

TESTS_C+=	t_setjmp
TESTS_C+=	t_sigstack
TESTS_C+=	t_threadjmp

LDFLAGS.t_threadjmp+= -pthread

.include <bsd.test.mk>
