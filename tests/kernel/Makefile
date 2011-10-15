# $NetBSD: Makefile,v 1.17 2011/10/15 06:33:45 jruoho Exp $

NOMAN=		# defined

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/kernel

TESTS_SUBDIRS=		kqueue tty

TESTS_C=	t_lock
TESTS_C+=	t_lwpctl
TESTS_C+=	t_pty
TESTS_C+=	t_rnd
TESTS_C+=	t_time
TESTS_C+=	t_ucontext
TESTS_C+=	t_writev
TESTS_C+=	t_extattrctl
TESTS_C+=	t_sigaction
TESTS_C+=	t_filedesc

TESTS_SH=	t_umount
TESTS_SH+=	t_ps_strings

BINDIR=		${TESTSDIR}
PROGS=		h_ps_strings1
PROGS+=		h_ps_strings2

LDADD.t_rnd+=  -lrumpvfs -lrumpdev_rnd -lrumpdev -lrump -lrumpuser -lpthread
LDADD.t_filedesc+=  -lrumpvfs -lrumpdev_rnd -lrumpdev -lrump -lrumpuser -lpthread
LDADD.t_extattrctl+= -lrumpvfs -lrump -lrumpuser -lpthread

.PATH:			${NETBSDSRCDIR}/sys/kern
TESTS_C+=		t_extent
SRCS.t_extent=		t_extent.c subr_extent.c
CPPFLAGS.t_extent.c=	-D_EXTENT_TESTING -D__POOL_EXPOSE
CPPFLAGS.subr_extent.c=	-D_EXTENT_TESTING -D__POOL_EXPOSE

.include <bsd.test.mk>
