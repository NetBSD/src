# $NetBSD: Makefile,v 1.2.10.1 2020/04/08 14:09:11 martin Exp $
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
