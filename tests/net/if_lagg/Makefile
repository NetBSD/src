# $NetBSD: Makefile,v 1.1.2.2 2021/05/31 22:15:23 cjep Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net/if_lagg

.for name in lagg
TESTS_SH+=		t_${name}
TESTS_SH_SRC_t_${name}=	../net_common.sh t_${name}.sh
.endfor

.include <bsd.test.mk>
