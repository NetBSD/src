# $NetBSD: Makefile,v 1.29 2022/12/01 04:24:37 ozaki-r Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net/net

TESTS_C=	t_bind
TESTS_C+=	t_unix
TESTS_C+=	t_mapped
TESTS_C+=	t_tcp
TESTS_C+=	t_udp
TESTS_C+=	t_pktinfo
.if (${MKRUMP} != "no") && !defined(BSD_MK_COMPAT_FILE)
TESTS_C+=	t_socket_afinet
TESTS_C+=	t_ip_reass
TESTS_C+=	t_pktinfo_send
TESTS_C+=	t_raw

.for name in forwarding ipaddress ipv6address ipv6_lifetime mtudisc mtudisc6 \
    ping_opts ping6_opts
TESTS_SH+=		t_${name}
TESTS_SH_SRC_t_${name}=	../net_common.sh t_${name}.sh
.endfor
.endif

.PATH:				${.CURDIR}/../../../lib/libc/gen
CPPFLAGS.sysctlbyname.c+=	-DRUMP_ACTION
OBJS.t_ip_reass+=		sysctlbyname.o

LDADD.t_pktinfo_send+=	-lrumpnet_local -lrumpnet_netinet -lrumpnet_net
LDADD.t_pktinfo_send+=	-lrumpnet_shmif -lrumpnet
LDADD.t_pktinfo_send+=	${LIBRUMPBASE}
LDADD.t_raw+=	-lrumpnet_local -lrumpnet_netinet -lrumpnet_net
LDADD.t_raw+=	-lrumpnet ${LIBRUMPBASE}
LDADD.t_socket_afinet+=	-lrumpdev_bpf -lrumpdev -lrumpnet_netinet -lrumpnet_net
LDADD.t_socket_afinet+=	-lrumpnet_local -lrumpnet ${LIBRUMPBASE}
LDADD.t_ip_reass+=	-lrumpdev_bpf -lrumpdev -lrumpnet_netinet -lrumpnet_net
LDADD.t_ip_reass+=	-lrumpnet_local -lrumpnet ${LIBRUMPBASE}

LDADD.t_mapped+=	-lutil

.include <bsd.test.mk>
