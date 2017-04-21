# $NetBSD: Makefile,v 1.2.2.1 2017/04/21 16:54:12 bouyer Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net/if_bridge

.for name in bridge rtable
TESTS_SH+=		t_${name}
TESTS_SH_SRC_t_${name}=	../net_common.sh t_${name}.sh
.endfor

.include <bsd.test.mk>
