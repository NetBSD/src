#	$NetBSD: Makefile,v 1.7 2020/03/01 18:08:16 christos Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net/if_loop

TESTS_C=	t_pr

LDADD+=		-lrumpnet_netinet -lrumpnet_net -lrumpnet
LDADD+=		${LIBRUMPBASE}

.for name in basic
TESTS_SH+=		t_${name}
TESTS_SH_SRC_t_${name}=	../net_common.sh t_${name}.sh
.endfor

.include <bsd.test.mk>
