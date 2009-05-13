# $NetBSD: Makefile,v 1.1.2.2 2009/05/13 19:19:33 jym Exp $

NOMAN=		# defined

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/kernel

SUBDIR=		kqueue

TESTS_C=	t_lock
TESTS_C+=	t_lwpctl
TESTS_C+=	t_mkdir
TESTS_C+=	t_pipe
TESTS_C+=	t_poll3w
TESTS_C+=	t_posix_fadvise
TESTS_C+=	t_time
TESTS_C+=	t_ucontext
TESTS_C+=	t_writev

TESTS_SH=	t_umount

.PATH:			${NETBSDSRCDIR}/sys/kern
TESTS_C+=		t_extent
SRCS.t_extent=		t_extent.c subr_extent.c
CPPFLAGS.t_extent+=	-D_EXTENT_TESTING -D__POOL_EXPOSE

.include <bsd.test.mk>
.include <bsd.subdir.mk>
