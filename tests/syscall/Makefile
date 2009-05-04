# $NetBSD: Makefile,v 1.2 2009/05/04 01:26:49 pooka Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/syscall

TESTS_C+=	t_cmsg

LDADD.t_cmsg+=	-lrumpnet_local -lrumpnet_net -lrumpnet -lrumpvfs -lrump
LDADD.t_cmsg+=	-lrumpuser -lpthread

.include <bsd.test.mk>
