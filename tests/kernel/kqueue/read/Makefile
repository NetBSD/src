# $NetBSD: Makefile,v 1.1.2.2 2009/05/13 19:19:34 jym Exp $

NOMAN=		# defined

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/kernel/kqueue/read

TESTS_C=	t_fifo
TESTS_C+=	t_file
TESTS_C+=	t_file2
TESTS_C+=	t_pipe
TESTS_C+=	t_ttypty

LDADD.t_ttypty=	-lutil

.include <bsd.test.mk>
