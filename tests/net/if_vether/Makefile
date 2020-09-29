# $NetBSD: Makefile,v 1.1 2020/09/29 19:41:48 roy Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net/if_vether

.for name in vether
TESTS_SH+=		t_${name}
TESTS_SH_SRC_t_${name}=	../net_common.sh t_${name}.sh
.endfor

.include <bsd.test.mk>
