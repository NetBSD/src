# $NetBSD: Makefile,v 1.5 2009/09/14 17:24:21 skrll Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/syscall

TESTS_C+=	t_cmsg

LDADD.t_cmsg+=	-lrumpnet_local -lrumpnet_net -lrumpnet
LDADD.t_cmsg+=	-lrumpfs_tmpfs -lrumpvfs -lrump -lrumpuser -lpthread

.include <bsd.test.mk>
