# $NetBSD: Makefile,v 1.7 2021/10/13 04:57:19 thorpej Exp $

WARNS?=6
NOMAN=		# defined

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/kernel/kqueue

TESTS_SUBDIRS=	read
TESTS_SUBDIRS+=	write

TESTS_C=	t_ioctl
TESTS_C+=	t_proc1
TESTS_C+=	t_proc2
TESTS_C+=	t_proc3
TESTS_C+=	t_proc4
TESTS_C+=	t_scan
TESTS_C+=	t_sig
TESTS_C+=	t_timer
TESTS_C+=	t_vnode

LDADD.t_scan+=	-lpthread

.include <bsd.test.mk>
