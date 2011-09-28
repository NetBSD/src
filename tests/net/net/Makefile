# $NetBSD: Makefile,v 1.3 2011/09/28 23:11:12 christos Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net/net

TESTS_C=	t_raw
TESTS_C+=	t_unix

LDADD.t_raw+=		-lrumpnet_local -lrumpnet_netinet -lrumpnet_net
LDADD.t_raw+=		-lrumpnet -lrumpvfs -lrump -lrumpuser -lpthread

.include <bsd.test.mk>
