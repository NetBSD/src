# $NetBSD: Makefile,v 1.9 2010/05/19 19:17:08 jruoho Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/syscall

TESTS_C+=	t_cmsg t_timer

LDADD+=	-lrumpnet_local -lrumpnet_net -lrumpnet
LDADD+=	-lrumpvfs -lrump -lrumpuser -lpthread

WARNS=		4

.include <bsd.test.mk>
