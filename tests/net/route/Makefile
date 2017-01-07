#	$NetBSD: Makefile,v 1.4.2.1 2017/01/07 08:56:56 pgoyette Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net/route

.for name in change flags flags6 route
TESTS_SH+=		t_${name}
TESTS_SH_SRC_t_${name}=	../net_common.sh t_${name}.sh
.endfor


.include <bsd.test.mk>
