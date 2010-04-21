# $NetBSD: Makefile,v 1.1 2010/04/21 11:07:34 pooka Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net/bpf

TESTS_C=	t_div-by-zero

LDADD+=		-lrumpdev_bpf -lrumpdev -lrumpnet_net -lrumpnet -lrumpvfs
LDADD+=		-lrump -lrumpuser -lpthread

.include <bsd.test.mk>
