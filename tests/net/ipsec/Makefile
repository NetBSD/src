# $NetBSD: Makefile,v 1.3 2017/04/27 06:53:44 ozaki-r Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net/ipsec

.for name in ipsec_ah_keys ipsec_esp_keys ipsec_gif ipsec_l2tp ipsec_sysctl \
    ipsec_transport ipsec_tunnel
TESTS_SH+=		t_${name}
TESTS_SH_SRC_t_${name}=	../net_common.sh ./algorithms.sh t_${name}.sh
.endfor

.include <bsd.test.mk>
