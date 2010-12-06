# $NetBSD: Makefile,v 1.2 2010/12/06 11:32:01 pooka Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net/bpf

TESTS_C=	t_bpf
TESTS_C+=	t_div-by-zero

LDADD+=		-lrumpnet_shmif
LDADD+=		-lrumpdev_bpf -lrumpdev -lrumpnet_net -lrumpnet -lrumpvfs
LDADD+=		-lrump -lrumpuser -lpthread

.include <bsd.test.mk>
