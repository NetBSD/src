# $NetBSD: Makefile,v 1.1 2021/07/14 03:22:33 ozaki-r Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net/altq

.for name in cbq
 TESTS_SH+=	t_${name}
 TESTS_SH_SRC_t_${name}=	../net_common.sh t_${name}.sh
.endfor

.include <bsd.test.mk>
