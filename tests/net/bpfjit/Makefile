# $NetBSD: Makefile,v 1.9 2020/03/01 18:08:15 christos Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net/bpfjit

TESTS_C=	t_bpfjit
TESTS_C+=	t_cop
TESTS_C+=	t_extmem
TESTS_C+=	t_mbuf

PAXCTL_FLAGS=	+m

LDADD+=		-lrumpnet_bpfjit -lrumpkern_sljit
LDADD+=		-lrumpdev_bpf -lrumpnet_net -lrumpnet
LDADD+=		${LIBRUMPBASE}

.include <bsd.test.mk>
