#	$NetBSD: Makefile,v 1.6 2019/06/01 06:59:17 kre Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net/if_loop

TESTS_C=	t_pr

LDADD+=		-lrumpnet_netinet -lrumpnet_net -lrumpnet
LDADD+=		-lrump -lrumpvfs -lrumpuser -lrump -lpthread

.for name in basic
TESTS_SH+=		t_${name}
TESTS_SH_SRC_t_${name}=	../net_common.sh t_${name}.sh
.endfor

.include <bsd.test.mk>
