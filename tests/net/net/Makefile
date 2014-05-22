# $NetBSD: Makefile,v 1.3.2.2 2014/05/22 11:42:22 yamt Exp $
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

LDADD.t_raw+=		-lrumpnet_local -lrumpnet_netinet -lrumpnet_net
LDADD.t_raw+=		-lrumpnet -lrumpvfs -lrump -lrumpuser -lpthread

.include <bsd.test.mk>
