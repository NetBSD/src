#	$NetBSD: Makefile,v 1.4 2016/08/08 14:34:02 pgoyette Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net/carp

TESTS_C=	t_basic

LDADD+=		-lrumpnet_shmif -lrumpnet_netinet -lrumpnet_net -lrumpdev
LDADD+=		-lrumpnet -lrump -lrumpuser -lrump_vfs -lpthread

.include <bsd.test.mk>
