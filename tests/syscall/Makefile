# $NetBSD: Makefile,v 1.25 2011/05/01 09:09:35 jruoho Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/syscall

TESTS_C+=	t_access t_cmsg t_dup t_fsync
TESTS_C+=	t_getgroups t_getpid t_getrusage t_getsid t_gettimeofday
TESTS_C+=	t_kill t_mmap t_mprotect t_msync t_nanosleep
TESTS_C+=	t_setrlimit t_setuid t_timer t_umask

LDADD.t_getpid+=	-lpthread
LDADD.t_timer+=		-lpthread

LDADD.t_cmsg+=		-lrumpnet_local -lrumpnet_net -lrumpnet
LDADD.t_cmsg+=		-lrumpvfs -lrump -lrumpuser -lpthread

WARNS=		4

.include <bsd.test.mk>
