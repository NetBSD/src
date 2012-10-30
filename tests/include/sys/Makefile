# $NetBSD: Makefile,v 1.5.2.2 2012/10/30 18:59:53 yamt Exp $

NOMAN=		# defined

.include <bsd.own.mk>

TESTSDIR=		${TESTSBASE}/include/sys

TESTS_C=		t_bitops t_bootblock t_cdefs t_tree t_types

LDADD.t_bitops+=	-lm

.if (${MKRUMP} != "no")
TESTS_C+= t_socket
LDADD.t_socket+=	-lrumpnet_local -lrumpnet_net -lrumpnet
LDADD.t_socket+=	-lrumpvfs -lrump -lrumpuser -lpthread
.endif

.include <bsd.test.mk>
