# $NetBSD: Makefile,v 1.1.4.2 2019/01/18 08:51:00 pgoyette Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net/if_ipsec

.for name in ipsec ipsec_natt ipsec_pfil
TESTS_SH+=		t_${name}
TESTS_SH_SRC_t_${name}=	../net_common.sh t_${name}.sh \
	../ipsec/common.sh ../ipsec/algorithms.sh
.endfor

.include <bsd.test.mk>
