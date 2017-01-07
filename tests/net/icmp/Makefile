# $NetBSD: Makefile,v 1.6.2.1 2017/01/07 08:56:55 pgoyette Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net/icmp

TESTS_C=	t_forward
TESTS_C+=	t_ping

.for name in ping2 icmp_redirect icmp6_redirect
TESTS_SH+=		t_${name}
TESTS_SH_SRC_t_${name}=	../net_common.sh t_${name}.sh
.endfor

LDADD+=		-lrumpnet_shmif -lrumpnet_netinet -lrumpnet_net -lrumpnet
LDADD+=		-lrump -lrumpuser -lrump -lpthread

.include <bsd.test.mk>
