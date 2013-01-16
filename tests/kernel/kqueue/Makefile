# $NetBSD: Makefile,v 1.2.6.1 2013/01/16 05:33:58 yamt Exp $

NOMAN=		# defined

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/kernel/kqueue

TESTS_SUBDIRS=		read
TESTS_SUBDIRS+=	write

TESTS_C=	t_ioctl
TESTS_C+=	t_proc1
TESTS_C+=	t_proc2
TESTS_C+=	t_proc3
TESTS_C+=	t_sig

.include <bsd.test.mk>
