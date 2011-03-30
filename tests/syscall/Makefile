# $NetBSD: Makefile,v 1.10 2011/03/30 09:43:21 jruoho Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/syscall

TESTS_C+=	t_cmsg t_fsync t_timer

LDADD.t_cmsg+=	-lrumpnet_local -lrumpnet_net -lrumpnet
LDADD.t_cmsg+=	-lrumpvfs -lrump -lrumpuser -lpthread

WARNS=		4

.include <bsd.test.mk>
