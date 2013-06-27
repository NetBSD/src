# $NetBSD: Makefile,v 1.5 2013/06/27 20:01:24 christos Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net/net

TESTS_C=	t_raw
TESTS_C+=	t_unix
TESTS_C+=	t_udp
TESTS_C+=	t_pktinfo

LDADD.t_raw+=		-lrumpnet_local -lrumpnet_netinet -lrumpnet_net
LDADD.t_raw+=		-lrumpnet -lrumpvfs -lrump -lrumpuser -lpthread

.include <bsd.test.mk>
