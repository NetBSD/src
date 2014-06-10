# $NetBSD: Makefile,v 1.4 2014/06/10 04:28:40 he Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net/bpf

TESTS_C=	t_bpf
TESTS_C+=	t_div-by-zero

LDADD+=		-lrumpnet_shmif
LDADD+=		-lrumpdev_bpf -lrumpdev -lrumpnet_netinet -lrumpnet_net
LDADD+=		-lrumpnet -lrumpvfs -lrump -lrumpuser -lrump -lpthread

.include <bsd.test.mk>
