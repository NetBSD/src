# $NetBSD: Makefile,v 1.35 2011/07/03 15:05:43 jruoho Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/syscall

TESTS_C+=	t_access t_cmsg t_dup t_fsync
TESTS_C+=	t_getgroups t_getlogin t_getpid
TESTS_C+=	t_getrusage t_getsid t_gettimeofday
TESTS_C+=	t_itimer t_kill
TESTS_C+=	t_mincore t_mknod t_mmap t_mprotect t_msync t_nanosleep
TESTS_C+=	t_poll t_pollts t_pselect
TESTS_C+=	t_setrlimit t_setuid t_stat t_timer t_truncate
TESTS_C+=	t_umask t_unlink

LDADD.t_getpid+=	-lpthread
LDADD.t_timer+=		-lpthread

LDADD.t_cmsg+=		-lrumpnet_local -lrumpnet_net -lrumpnet
LDADD.t_cmsg+=		-lrumpvfs -lrump -lrumpuser -lpthread

WARNS=		4

.include <bsd.test.mk>
