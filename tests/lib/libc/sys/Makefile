# $NetBSD: Makefile,v 1.10 2011/10/15 06:10:26 jruoho Exp $

MKMAN=	no

.include <bsd.own.mk>

.include "../arch/Makefile.exec_prot"

TESTSDIR=		${TESTSBASE}/lib/libc/sys

TESTS_C+=		t_access
TESTS_C+=		t_chroot
TESTS_C+=		t_clone
TESTS_C+=		t_dup
TESTS_C+=		t_fsync
TESTS_C+=		t_getcontext
TESTS_C+=		t_getgroups
TESTS_C+=		t_getitimer
TESTS_C+=		t_getlogin
TESTS_C+=		t_getpid
TESTS_C+=		t_getrusage
TESTS_C+=		t_getsid
TESTS_C+=		t_gettimeofday
TESTS_C+=		t_issetugid
TESTS_C+=		t_kill
TESTS_C+=		t_link
TESTS_C+=		t_mincore
TESTS_C+=		t_mkfifo
TESTS_C+=		t_mknod
TESTS_C+=		t_mmap
TESTS_C+=		t_mprotect
TESTS_C+=		t_msync
TESTS_C+=		t_nanosleep
TESTS_C+=		t_posix_fadvise
TESTS_C+=		t_poll
TESTS_C+=		t_revoke
TESTS_C+=		t_select
TESTS_C+=		t_setrlimit
TESTS_C+=		t_setuid
TESTS_C+=		t_sigqueue
TESTS_C+=		t_stat
TESTS_C+=		t_timer_create
TESTS_C+=		t_truncate
TESTS_C+=		t_umask
TESTS_C+=		t_unlink

SRCS.t_mprotect=	t_mprotect.c ${SRCS_EXEC_PROT}

LDADD.t_getpid+=        -lpthread
LDADD.t_posix_fadvise+= -lrumpvfs -lrump -lrumpuser -lpthread

WARNS=			4

.include <bsd.test.mk>

