# $NetBSD: Makefile,v 1.28.2.1 2011/06/23 14:20:41 cherry Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/syscall

TESTS_C+=	t_access t_cmsg t_dup t_fsync
TESTS_C+=	t_getgroups t_getlogin t_getpid
TESTS_C+=	t_getrusage t_getsid t_gettimeofday
TESTS_C+=	t_itimer t_kill
TESTS_C+=	t_mincore t_mmap t_mprotect t_msync t_nanosleep
TESTS_C+=	t_poll t_pollts t_pselect
TESTS_C+=	t_setrlimit t_setuid t_stat t_timer t_truncate t_umask

LDADD.t_getpid+=	-lpthread
LDADD.t_timer+=		-lpthread

LDADD.t_cmsg+=		-lrumpnet_local -lrumpnet_net -lrumpnet
LDADD.t_cmsg+=		-lrumpvfs -lrump -lrumpuser -lpthread

WARNS=		4

.include <bsd.test.mk>
