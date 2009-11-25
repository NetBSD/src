# $NetBSD: Makefile,v 1.7 2009/11/25 16:17:11 pooka Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/syscall

TESTS_C+=	t_cmsg

LDADD+=	-lrumpnet_local -lrumpnet_net -lrumpnet
LDADD+=	-lrumpfs_tmpfs -lrumpvfs -lrump -lrumpuser -lpthread

WARNS=		4

.include <bsd.test.mk>
