# $NetBSD: Makefile,v 1.1 2010/07/04 19:30:59 pooka Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net/icmp

TESTS_C=	t_forward

LDADD+=		-lrumpnet_shmif -lrumpnet_netinet -lrumpnet_net -lrumpnet
LDADD+=		-lrump -lrumpuser -lpthread

.include <bsd.test.mk>
