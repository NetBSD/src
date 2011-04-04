# $NetBSD: Makefile,v 1.14 2011/04/04 06:48:05 jruoho Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/syscall

TESTS_C+=	t_access t_cmsg t_dup t_fsync t_getpid
TESTS_C+=	t_mmap t_mprotect t_timer

LDADD.t_getpid+=-lpthread

LDADD.t_cmsg+=	-lrumpnet_local -lrumpnet_net -lrumpnet
LDADD.t_cmsg+=	-lrumpvfs -lrump -lrumpuser -lpthread

WARNS=		4

.include <bsd.test.mk>
