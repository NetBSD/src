# $NetBSD: Makefile,v 1.1.2.2 2009/05/13 19:19:33 jym Exp $

NOMAN=		# defined

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/kernel/kqueue

SUBDIR=		read
SUBDIR+=	write

TESTS_C=	t_ioctl
TESTS_C+=	t_proc1
TESTS_C+=	t_proc2
TESTS_C+=	t_sig

.include <bsd.test.mk>
.include <bsd.subdir.mk>
