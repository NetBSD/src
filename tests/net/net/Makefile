# $NetBSD: Makefile,v 1.11 2015/06/22 00:05:23 matt Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net/net

TESTS_C=	t_unix
TESTS_C+=	t_tcp
TESTS_C+=	t_udp
TESTS_C+=	t_pktinfo
.if (${MKRUMP} != "no") && !defined(BSD_MK_COMPAT_FILE)
TESTS_C+=	t_raw
TESTS_SH=	t_forwarding
.endif

LDADD.t_raw+=	-lrumpnet_local -lrumpnet_netinet -lrumpnet_net
LDADD.t_raw+=	-lrumpnet -lrumpvfs -lrump -lrumpuser -lrump -lpthread

.include <bsd.test.mk>
