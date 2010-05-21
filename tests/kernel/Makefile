# $NetBSD: Makefile,v 1.4 2010/05/21 16:47:45 pooka Exp $

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
TESTS_C+=	t_rnd
TESTS_C+=	t_time
TESTS_C+=	t_ucontext
TESTS_C+=	t_writev
TESTS_C+=	t_extattrctl

TESTS_SH=	t_umount

LDADD.t_rnd+=  -lrumpvfs -lrumpdev_rnd -lrumpdev -lrump -lrumpuser -lpthread
LDADD.t_extattrctl+= -lrumpvfs -lrump -lrumpuser -lpthread

.PATH:			${NETBSDSRCDIR}/sys/kern
TESTS_C+=		t_extent
SRCS.t_extent=		t_extent.c subr_extent.c
CPPFLAGS.t_extent.c=	-D_EXTENT_TESTING -D__POOL_EXPOSE
CPPFLAGS.subr_extent.c=	-D_EXTENT_TESTING -D__POOL_EXPOSE

.include <bsd.test.mk>
.include <bsd.subdir.mk>
