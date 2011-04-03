# $NetBSD: Makefile,v 1.13 2011/04/03 16:12:45 jruoho Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/syscall

TESTS_C+=	t_access t_cmsg t_dup t_fsync t_mmap t_mprotect t_timer

LDADD.t_cmsg+=	-lrumpnet_local -lrumpnet_net -lrumpnet
LDADD.t_cmsg+=	-lrumpvfs -lrump -lrumpuser -lpthread

WARNS=		4

.include <bsd.test.mk>
