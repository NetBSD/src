# $NetBSD: Makefile,v 1.19 2011/04/05 19:02:23 jruoho Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/syscall

TESTS_C+=	t_access t_cmsg t_dup t_fsync t_getpid t_getsid
TESTS_C+=	t_mmap t_mprotect t_setrlimit t_setuid t_timer t_umask

LDADD.t_getpid+=	-lpthread
LDADD.t_timer+=		-lpthread

LDADD.t_cmsg+=		-lrumpnet_local -lrumpnet_net -lrumpnet
LDADD.t_cmsg+=		-lrumpvfs -lrump -lrumpuser -lpthread

WARNS=		4

.include <bsd.test.mk>
