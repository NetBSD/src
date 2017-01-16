#	$NetBSD: Makefile,v 1.6 2017/01/16 08:18:11 ozaki-r Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net/carp

.for name in basic
TESTS_SH+=		t_${name}
TESTS_SH_SRC_t_${name}=	../net_common.sh t_${name}.sh
.endfor

.include <bsd.test.mk>
