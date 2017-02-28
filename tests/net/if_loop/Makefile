#	$NetBSD: Makefile,v 1.4 2017/02/28 03:32:11 ozaki-r Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net/if_loop

TESTS_C=	t_pr

LDADD+=		-lrumpnet_netinet -lrumpnet_net -lrumpnet
LDADD+=		-lrump -lrumpuser -lrump -lpthread -lrumpdev -lrumpvfs

.for name in basic
TESTS_SH+=		t_${name}
TESTS_SH_SRC_t_${name}=	../net_common.sh t_${name}.sh
.endfor

.include <bsd.test.mk>
