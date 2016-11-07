# $NetBSD: Makefile,v 1.18 2016/11/07 00:54:48 ozaki-r Exp $
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
TESTS_SH+=	t_ipaddress
TESTS_SH+=	t_ipv6address
TESTS_SH+=	t_ipv6_lifetime
TESTS_SH+=	t_mtudisc
TESTS_SH+=	t_mtudisc6
TESTS_SH+=	t_ping6_opts
.endif

LDADD.t_raw+=	-lrumpnet_local -lrumpnet_netinet -lrumpnet_net -lrumpdev
LDADD.t_raw+=	-lrumpnet -lrumpvfs -lrump -lrumpuser -lrump -lpthread

.include <bsd.test.mk>
