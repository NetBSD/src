# $NetBSD: Makefile,v 1.7.2.1 2014/08/10 06:57:30 tls Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net/net

TESTS_C=	t_unix
TESTS_C+=	t_tcp
TESTS_C+=	t_udp
TESTS_C+=	t_pktinfo
.if (${MKRUMP} != "no")
TESTS_C+=	t_raw
.endif

LDADD.t_raw+=	-lrumpnet_local -lrumpnet_netinet -lrumpnet_net
LDADD.t_raw+=	-lrumpnet -lrumpvfs -lrump -lrumpuser -lrump -lpthread

.include <bsd.test.mk>
