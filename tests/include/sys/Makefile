# $NetBSD: Makefile,v 1.9 2016/01/23 21:22:48 christos Exp $

NOMAN=		# defined

.include <bsd.own.mk>

TESTSDIR=		${TESTSBASE}/include/sys

TESTS_C=		t_bitops t_bootblock t_cdefs t_tree t_types

LDADD.t_bitops+=	-lm

.if (${MKRUMP} != "no") && !defined(BSD_MK_COMPAT_FILE)
TESTS_C+= t_socket
CPPFLAGS.t_socket.c+=	-D_KERNTYPES
LDADD.t_socket+=	-lrumpnet_local -lrumpnet_net -lrumpnet
LDADD.t_socket+=	-lrumpvfs -lrump -lrumpuser -lpthread
.endif

.include <bsd.test.mk>
