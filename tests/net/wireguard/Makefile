# $NetBSD: Makefile,v 1.1 2020/08/20 21:28:01 riastradh Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net/wireguard

.for name in basic interoperability misc tunnel
TESTS_SH+=		t_${name}
TESTS_SH_SRC_t_${name}=	../net_common.sh ./common.sh t_${name}.sh
.endfor

.include <bsd.test.mk>
