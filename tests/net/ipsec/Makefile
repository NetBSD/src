# $NetBSD: Makefile,v 1.4 2017/05/09 04:25:28 ozaki-r Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net/ipsec

.for name in ipsec_ah_keys ipsec_esp_keys ipsec_gif ipsec_l2tp ipsec_sysctl \
    ipsec_transport ipsec_tunnel
TESTS_SH+=		t_${name}
TESTS_SH_SRC_t_${name}=	../net_common.sh ./common.sh ./algorithms.sh \
    t_${name}.sh
.endfor

.include <bsd.test.mk>
