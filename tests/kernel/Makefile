# $NetBSD: Makefile,v 1.9 2010/11/11 17:36:57 pooka Exp $

NOMAN=		# defined

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/kernel

TESTS_SUBDIRS=		kqueue tty

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
TESTS_C+=	t_sigaction
TESTS_C+=	t_filedesc

TESTS_SH=	t_umount

LDADD.t_rnd+=  -lrumpvfs -lrumpdev_rnd -lrumpdev -lrump -lrumpuser -lpthread
LDADD.t_filedesc+=  -lrumpvfs -lrumpdev_rnd -lrumpdev -lrump -lrumpuser -lpthread
LDADD.t_extattrctl+= -lrumpvfs -lrump -lrumpuser -lpthread
LDADD.t_posix_fadvise+= -lrumpvfs -lrump -lrumpuser -lpthread

.PATH:			${NETBSDSRCDIR}/sys/kern
TESTS_C+=		t_extent
SRCS.t_extent=		t_extent.c subr_extent.c
CPPFLAGS.t_extent.c=	-D_EXTENT_TESTING -D__POOL_EXPOSE
CPPFLAGS.subr_extent.c=	-D_EXTENT_TESTING -D__POOL_EXPOSE

.include <bsd.test.mk>
