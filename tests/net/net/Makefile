# $NetBSD: Makefile,v 1.3.8.1 2013/02/25 00:30:24 tls Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net/net

TESTS_C=	t_raw
TESTS_C+=	t_unix
TESTS_C+=	t_udp

LDADD.t_raw+=		-lrumpnet_local -lrumpnet_netinet -lrumpnet_net
LDADD.t_raw+=		-lrumpnet -lrumpvfs -lrump -lrumpuser -lpthread

.include <bsd.test.mk>
