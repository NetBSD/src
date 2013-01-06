# $NetBSD: Makefile,v 1.4 2013/01/06 00:35:22 christos Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net/net

TESTS_C=	t_raw
TESTS_C+=	t_unix
TESTS_C+=	t_udp

LDADD.t_raw+=		-lrumpnet_local -lrumpnet_netinet -lrumpnet_net
LDADD.t_raw+=		-lrumpnet -lrumpvfs -lrump -lrumpuser -lpthread

.include <bsd.test.mk>
