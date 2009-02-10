# $NetBSD: Makefile,v 1.1 2009/02/10 13:43:54 pooka Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/syscall

TESTS_C=	t_cmsg

LDADD+=		-lrumpnet_local -lrumpnet_net -lrumpnet -lrumpvfs -lrump
LDADD+=		-lrumpuser -lpthread

.include <bsd.test.mk>
