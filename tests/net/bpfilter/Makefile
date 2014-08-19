# $NetBSD: Makefile,v 1.1.2.1 2014/08/20 00:04:51 tls Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net/bpfilter

TESTS_C=	t_bpfilter

LDADD+=		-lrumpnet_shmif
LDADD+=		-lrumpdev_bpf -lrumpdev -lrumpnet_netinet -lrumpnet_net
LDADD+=		-lrumpnet -lrumpvfs -lrump -lrumpuser -lrump -lpthread

.include <bsd.test.mk>
