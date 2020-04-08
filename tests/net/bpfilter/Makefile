# $NetBSD: Makefile,v 1.2.24.1 2020/04/08 14:09:11 martin Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net/bpfilter

TESTS_C=	t_bpfilter

LDADD+=		-lrumpnet_shmif
LDADD+=		-lrumpdev_bpf -lrumpdev -lrumpnet_netinet -lrumpnet_net
LDADD+=		-lrumpnet ${LIBRUMPBASE}

.include <bsd.test.mk>
