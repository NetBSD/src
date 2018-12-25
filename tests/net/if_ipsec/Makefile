# $NetBSD: Makefile,v 1.2 2018/12/25 03:54:44 knakahara Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net/if_ipsec

.for name in ipsec ipsec_natt
TESTS_SH+=		t_${name}
TESTS_SH_SRC_t_${name}=	../net_common.sh t_${name}.sh \
	../ipsec/common.sh ../ipsec/algorithms.sh
.endfor

.include <bsd.test.mk>
