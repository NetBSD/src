# $NetBSD: Makefile,v 1.1 2016/12/02 05:54:15 kamil Exp $

NOMAN=		# defined

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/kernel/arch/amd64

TESTS_C+=	t_ptrace_wait
TESTS_C+=	t_ptrace_wait3
TESTS_C+=	t_ptrace_wait4
TESTS_C+=	t_ptrace_wait6
TESTS_C+=	t_ptrace_waitid
TESTS_C+=	t_ptrace_waitpid

.include <bsd.test.mk>
