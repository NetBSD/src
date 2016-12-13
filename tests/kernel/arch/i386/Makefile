# $NetBSD: Makefile,v 1.1 2016/12/13 18:00:10 kamil Exp $

NOMAN=		# defined

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/kernel/arch/i386

TESTS_C+=	t_ptrace_wait
TESTS_C+=	t_ptrace_wait3
TESTS_C+=	t_ptrace_wait4
TESTS_C+=	t_ptrace_wait6
TESTS_C+=	t_ptrace_waitid
TESTS_C+=	t_ptrace_waitpid

CPPFLAGS+=	-D_KERNTYPES

.include <bsd.test.mk>
