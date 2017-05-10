# $NetBSD: Makefile,v 1.5 2017/05/10 09:08:25 ozaki-r Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net/ipsec

.for name in ipsec_ah_keys ipsec_esp_keys ipsec_gif ipsec_l2tp ipsec_sysctl \
    ipsec_transport ipsec_tunnel ipsec_tunnel_odd
TESTS_SH+=		t_${name}
TESTS_SH_SRC_t_${name}=	../net_common.sh ./common.sh ./algorithms.sh \
    t_${name}.sh
.endfor

.include <bsd.test.mk>
