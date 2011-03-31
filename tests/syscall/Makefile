# $NetBSD: Makefile,v 1.11 2011/03/31 13:07:20 jruoho Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/syscall

TESTS_C+=	t_cmsg t_fsync t_mmap t_timer

LDADD.t_cmsg+=	-lrumpnet_local -lrumpnet_net -lrumpnet
LDADD.t_cmsg+=	-lrumpvfs -lrump -lrumpuser -lpthread

WARNS=		4

.include <bsd.test.mk>
