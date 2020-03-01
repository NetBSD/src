# $NetBSD: Makefile,v 1.3 2020/03/01 18:08:16 christos Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net/can

TESTS_C=	t_can t_canfilter

SRCS.t_can=		t_can.c h_canutils.c

SRCS.t_canfilter=	t_canfilter.c h_canutils.c

# XXX we don't use INET here, but we need rumpnet_netinet anyway:
# common code in if.c is compiled with -DINET and will dereference ip_pktq,
# which is NULL if rumpnet_netinet is not inclued.
# 
LDADD+=		-lrumpnet_netcan -lrumpnet_netinet -lrumpnet_net -lrumpnet
LDADD+=		${LIBRUMPBASE}

.include <bsd.test.mk>
