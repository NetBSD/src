# $NetBSD: Makefile,v 1.19.8.1 2023/09/04 16:57:56 martin Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/rump/rumpkern

TESTS_C=	t_copy
TESTS_C+=	t_kern
TESTS_C+=	t_lwproc
TESTS_C+=	t_modcmd
TESTS_C+=	t_modlinkset
TESTS_C+=	t_signals
TESTS_C+=	t_threads
TESTS_C+=	t_threadpool
TESTS_C+=	t_tsleep
TESTS_C+=	t_workqueue
TESTS_C+=	t_vm

TESTS_SH=	t_sp

SUBDIR+=	h_client h_server

LDADD.t_modlinkset+=	-lukfs -lrumpdev_disk -lrumpdev -lrumpfs_msdos 
LDADD.t_modlinkset+=	-lrumpfs_cd9660 ${LIBRUMPBASE}
LDADD+=			${LIBRUMPBASE}

PROGDPLIBS+=	kernspace	${.CURDIR}/../kernspace
LDADD+=		-lrump

WARNS=	4

.include <bsd.test.mk>
