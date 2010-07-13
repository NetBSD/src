# $NetBSD: Makefile,v 1.2 2010/07/13 21:13:25 jmmv Exp $

NOMAN=		# defined

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/kernel/kqueue

TESTS_SUBDIRS=		read
TESTS_SUBDIRS+=	write

TESTS_C=	t_ioctl
TESTS_C+=	t_proc1
TESTS_C+=	t_proc2
TESTS_C+=	t_sig

.include <bsd.test.mk>
