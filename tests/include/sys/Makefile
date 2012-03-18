# $NetBSD: Makefile,v 1.6 2012/03/18 15:30:59 christos Exp $

NOMAN=		# defined

.include <bsd.own.mk>

TESTSDIR=		${TESTSBASE}/include/sys

TESTS_C=		t_bitops t_bootblock t_cdefs t_socket t_tree t_types

LDADD.t_bitops+=	-lm
LDADD.t_socket+=	-lrumpnet_local -lrumpnet_net -lrumpnet
LDADD.t_socket+=	-lrumpvfs -lrump -lrumpuser -lpthread

.include <bsd.test.mk>
