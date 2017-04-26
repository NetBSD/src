# $NetBSD: Makefile,v 1.1.4.2 2017/04/26 02:53:34 pgoyette Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net/ipsec

.for name in ipsec_ah_keys ipsec_esp_keys ipsec_sysctl ipsec_transport \
    ipsec_tunnel
TESTS_SH+=		t_${name}
TESTS_SH_SRC_t_${name}=	../net_common.sh ./algorithms.sh t_${name}.sh
.endfor

.include <bsd.test.mk>
