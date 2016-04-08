# $NetBSD: Makefile,v 1.10 2016/04/08 21:53:10 riastradh Exp $

NOMAN=		# defined

.include <bsd.own.mk>

TESTSDIR=		${TESTSBASE}/include/sys

TESTS_C+=		t_bitops
TESTS_C+=		t_bootblock
TESTS_C+=		t_cdefs
TESTS_C+=		t_tree
TESTS_C+=		t_types

LDADD.t_bitops+=	-lm

.if (${MKRUMP} != "no") && !defined(BSD_MK_COMPAT_FILE)
TESTS_C+= t_socket
CPPFLAGS.t_socket.c+=	-D_KERNTYPES
LDADD.t_socket+=	-lrumpnet_local -lrumpnet_net -lrumpnet
LDADD.t_socket+=	-lrumpvfs -lrump -lrumpuser -lpthread
.endif

.include <bsd.test.mk>
