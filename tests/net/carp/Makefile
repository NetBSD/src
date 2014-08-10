#	$NetBSD: Makefile,v 1.1.22.1 2014/08/10 06:57:30 tls Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net/carp

TESTS_C=	t_basic

LDADD+=		-lrumpnet_shmif -lrumpnet_netinet -lrumpnet_net
LDADD+=		-lrumpnet -lrump -lrumpuser -lrump -lpthread

.include <bsd.test.mk>
