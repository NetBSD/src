# $NetBSD: Makefile,v 1.26 2022/11/17 08:45:36 ozaki-r Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net/net

TESTS_C=	t_bind
TESTS_C+=	t_unix
TESTS_C+=	t_mapped
TESTS_C+=	t_tcp
TESTS_C+=	t_udp
TESTS_C+=	t_pktinfo
TESTS_C+=	t_socket_afinet
.if (${MKRUMP} != "no") && !defined(BSD_MK_COMPAT_FILE)
TESTS_C+=	t_pktinfo_send
TESTS_C+=	t_raw

.for name in forwarding ipaddress ipv6address ipv6_lifetime mtudisc mtudisc6 \
    ping_opts ping6_opts
TESTS_SH+=		t_${name}
TESTS_SH_SRC_t_${name}=	../net_common.sh t_${name}.sh
.endfor
.endif

LDADD.t_pktinfo_send+=	-lrumpnet_local -lrumpnet_netinet -lrumpnet_net
LDADD.t_pktinfo_send+=	-lrumpnet_shmif -lrumpnet
LDADD.t_pktinfo_send+=	${LIBRUMPBASE}
LDADD.t_raw+=	-lrumpnet_local -lrumpnet_netinet -lrumpnet_net
LDADD.t_raw+=	-lrumpnet ${LIBRUMPBASE}
LDADD.t_socket_afinet+=	-lrumpdev_bpf -lrumpdev -lrumpnet_netinet -lrumpnet_net
LDADD.t_socket_afinet+=	-lrumpnet_local -lrumpnet ${LIBRUMPBASE}

LDADD.t_mapped+=	-lutil

.include <bsd.test.mk>
