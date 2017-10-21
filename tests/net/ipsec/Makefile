# $NetBSD: Makefile,v 1.6.2.1 2017/10/21 19:43:55 snj Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net/ipsec

.for name in ipsec_ah_keys ipsec_esp_keys ipsec_gif ipsec_l2tp ipsec_misc \
    ipsec_sockopt ipsec_sysctl ipsec_tcp ipsec_transport ipsec_tunnel \
    ipsec_tunnel_ipcomp ipsec_tunnel_odd
TESTS_SH+=		t_${name}
TESTS_SH_SRC_t_${name}=	../net_common.sh ./common.sh ./algorithms.sh \
    t_${name}.sh
.endfor

.include <bsd.test.mk>
