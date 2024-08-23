# $NetBSD: Makefile,v 1.2.54.1 2024/08/23 16:56:29 martin Exp $

NOMAN=		# defined

.include <bsd.own.mk>

WARNS=	4

TESTSDIR=	${TESTSBASE}/lib/libc/setjmp

TESTS_C+=	t_setjmp
TESTS_C+=	t_sigstack
TESTS_C+=	t_threadjmp

LDFLAGS.t_threadjmp+= -pthread

.include <bsd.test.mk>
