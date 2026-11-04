# $NetBSD: Makefile,v 1.6.10.1 2026/11/04 23:28:20 perseant Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net/bpf

TESTS_C=	t_bpf
TESTS_C+=	t_div-by-zero
TESTS_C+=	t_mbuf

LDADD+=		-lrumpnet_shmif
LDADD+=		-lrumpdev_bpf -lrumpdev -lrumpnet_netinet -lrumpnet_net
LDADD+=		-lrumpnet ${LIBRUMPBASE}

COPTS.t_bpf.c+=	${CC_WNO_CALLOC_TRANSPOSED_ARGS}

.include <bsd.test.mk>
