# $NetBSD: Makefile,v 1.3 2014/12/08 04:23:03 ozaki-r Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net/if

TESTS_C=	t_compat
TESTS_SH=	t_ifconf

PROGS=		ifconf
MAN.ifconf=	# empty
BINDIR.ifconf=	${TESTSDIR}

LDADD.t_compat=	-lrumpnet_shmif -lrumpnet_netinet -lrumpnet_net -lrumpnet -lrump
LDADD.t_compat=	-lrumpuser -lrump -lpthread

.include <bsd.test.mk>
