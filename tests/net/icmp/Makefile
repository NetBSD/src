# $NetBSD: Makefile,v 1.2 2010/08/09 15:08:43 pooka Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net/icmp

TESTS_C=	t_forward
TESTS_C+=	t_ping

LDADD+=		-lrumpnet_shmif -lrumpnet_netinet -lrumpnet_net -lrumpnet
LDADD+=		-lrump -lrumpuser -lpthread

.include <bsd.test.mk>
