# $NetBSD: Makefile,v 1.3.22.1 2014/08/10 06:57:30 tls Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net/bpf

TESTS_C=	t_bpf
TESTS_C+=	t_div-by-zero
TESTS_C+=	t_mbuf

LDADD+=		-lrumpnet_shmif
LDADD+=		-lrumpdev_bpf -lrumpdev -lrumpnet_netinet -lrumpnet_net
LDADD+=		-lrumpnet -lrumpvfs -lrump -lrumpuser -lrump -lpthread

.include <bsd.test.mk>
