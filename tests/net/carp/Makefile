#	$NetBSD: Makefile,v 1.1.12.1 2014/08/20 00:04:51 tls Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net/carp

TESTS_C=	t_basic

LDADD+=		-lrumpnet_shmif -lrumpnet_netinet -lrumpnet_net
LDADD+=		-lrumpnet -lrump -lrumpuser -lrump -lpthread

.include <bsd.test.mk>
