# $NetBSD: Makefile,v 1.6 2026/06/18 09:26:45 yamaguchi Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net/if_pppoe

.for name in pppoe pppoe_unnumbered pppoe_ipcp pppoe_ipv6cp \
             pppoe_tags pppoe_keepalive
TESTS_SH+=		t_${name}
TESTS_SH_SRC_t_${name}=	../net_common.sh common.sh t_${name}.sh
.endfor

.include <bsd.test.mk>
