# $NetBSD: Makefile,v 1.3 2009/05/07 16:19:30 pooka Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/syscall

TESTS_C+=	t_cmsg

LDADD.t_cmsg+=	-lrumpnet_local -lrumpnet_net -lrumpnet
LDADD.t_cmsg+=	-lrumpfs_tmpfs -lrumpvfs -lrump -lrumpuser -lpthread

.include <bsd.test.mk>
