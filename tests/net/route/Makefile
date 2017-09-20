#	$NetBSD: Makefile,v 1.6 2017/09/20 09:36:20 ozaki-r Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net/route

.for name in change flags flags6 route rtcache
TESTS_SH+=		t_${name}
TESTS_SH_SRC_t_${name}=	../net_common.sh t_${name}.sh
.endfor


.include <bsd.test.mk>
