# $NetBSD: Makefile,v 1.4 2009/08/21 14:41:22 skrll Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/syscall

TESTS_C+=	t_cmsg

LDADD.t_cmsg+=	-lrumpnet_local -lrumpnet_net -lrumpnet
LDADD.t_cmsg+=	-lrumpfs_tmpfs -lrumpvfs -lrump -lrumpuser -lpthread

LDFLAGS+=      -Wl,--ignore-unresolved-symbol,__start_link_set_domains
LDFLAGS+=      -Wl,--ignore-unresolved-symbol,__stop_link_set_domains
 
.include <bsd.test.mk>
