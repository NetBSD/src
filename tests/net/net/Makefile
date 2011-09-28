# $NetBSD: Makefile,v 1.2 2011/09/28 16:13:03 christos Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net/net

TESTS_C=	t_raw
TESTS_C=	t_unix

LDADD.t_raw+=		-lrumpnet_local -lrumpnet_netinet -lrumpnet_net
LDADD.t_raw+=		-lrunnet -lrumpvfs -lrump -lrumpuser -lpthread

.include <bsd.test.mk>
