# $NetBSD: Makefile,v 1.1.8.1 2018/02/26 00:41:14 snj Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net/if_tun

.for name in tun
TESTS_SH+=		t_${name}
TESTS_SH_SRC_t_${name}=	../net_common.sh t_${name}.sh
.endfor

.include <bsd.test.mk>
