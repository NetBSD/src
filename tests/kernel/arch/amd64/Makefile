# $NetBSD: Makefile,v 1.2 2016/12/13 12:59:46 kamil Exp $

NOMAN=		# defined

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/kernel/arch/amd64

TESTS_C+=	t_ptrace_wait
TESTS_C+=	t_ptrace_wait3
TESTS_C+=	t_ptrace_wait4
TESTS_C+=	t_ptrace_wait6
TESTS_C+=	t_ptrace_waitid
TESTS_C+=	t_ptrace_waitpid

CPPFLAGS+=	-D_KERNTYPES

.include <bsd.test.mk>
