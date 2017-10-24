#	$NetBSD: Makefile,v 1.5.6.1 2017/10/24 08:55:56 snj Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net/route

.for name in change flags flags6 route rtcache
TESTS_SH+=		t_${name}
TESTS_SH_SRC_t_${name}=	../net_common.sh t_${name}.sh
.endfor


.include <bsd.test.mk>
