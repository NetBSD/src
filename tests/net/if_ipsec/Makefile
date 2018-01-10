# $NetBSD: Makefile,v 1.1 2018/01/10 11:06:06 knakahara Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net/if_ipsec

.for name in ipsec
TESTS_SH+=		t_${name}
TESTS_SH_SRC_t_${name}=	../net_common.sh t_${name}.sh \
	../ipsec/common.sh ../ipsec/algorithms.sh
.endfor

.include <bsd.test.mk>
