# $NetBSD: Makefile,v 1.1.12.1 2014/08/20 00:04:52 tls Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net/if

TESTS_C=	t_compat

LDADD+=		-lrumpnet_shmif -lrumpnet_netinet -lrumpnet_net -lrumpnet -lrump
LDADD+=		-lrumpuser -lrump -lpthread

.include <bsd.test.mk>
