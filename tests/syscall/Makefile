# $NetBSD: Makefile,v 1.3.2.2 2009/05/13 19:19:34 jym Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/syscall

TESTS_C=	t_cmsg

LDADD+=		-lrumpnet_local -lrumpnet_net -lrumpnet -lrumpvfs -lrump
LDADD+=		-lrumpuser -lpthread

.include <bsd.test.mk>
