# $NetBSD: Makefile,v 1.1 2011/01/11 10:51:45 pooka Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net/net

TESTS_C=	t_raw

LDADD+=		-lrumpnet_local -lrumpnet_netinet -lrumpnet_net -lrumpnet
LDADD+=		-lrumpvfs -lrump -lrumpuser -lpthread

.include <bsd.test.mk>
