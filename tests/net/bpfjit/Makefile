# $NetBSD: Makefile,v 1.2 2014/06/30 21:34:22 alnsn Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net/bpfjit

TESTS_C=	t_bpfjit

LDADD+=		-lrumpnet_bpfjit -lrumpkern_sljit
LDADD+=		-lrumpdev_bpf -lrumpnet_net -lrumpnet
LDADD+=		-lrump -lrumpuser -lpthread

.include <bsd.test.mk>
