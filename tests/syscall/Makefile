# $NetBSD: Makefile,v 1.8 2009/11/26 17:33:23 pooka Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/syscall

TESTS_C+=	t_cmsg

LDADD+=	-lrumpnet_local -lrumpnet_net -lrumpnet
LDADD+=	-lrumpvfs -lrump -lrumpuser -lpthread

WARNS=		4

.include <bsd.test.mk>
