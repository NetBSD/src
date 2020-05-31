# $NetBSD: Makefile,v 1.16 2020/05/31 16:36:07 kamil Exp $

NOMAN=		# defined

.include <bsd.own.mk>

TESTSDIR=		${TESTSBASE}/include/sys

TESTS_C+=		t_bitops
TESTS_C+=		t_bootblock
TESTS_C+=		t_cdefs
TESTS_C+=		t_list
TESTS_C+=		t_pslist
TESTS_C+=		t_tree
TESTS_C+=		t_types

# NULL + 0 arithmetic raises LLVM UBSan warnings, specially in sys/pslist.h
# in the type-safe macros _PSLIST_VALIDATE_PTRS and _PSLIST_VALIDATE_CONTAINER.
# See also src/sys/rump/Makefile.rump
.if ${MKSANITIZER:Uno} == "yes" || ${MKLIBCSANITIZER:Uno} == "yes"
COPTS.t_pslist.c+=	${${ACTIVE_CC} == "clang":? -fno-delete-null-pointer-checks :}
.endif

CPPFLAGS.t_pslist.c+=	-I${NETBSDSRCDIR}/sys
LDADD.t_bitops+=	-lm

.if (${MKRUMP} != "no") && !defined(BSD_MK_COMPAT_FILE)
TESTS_C+= t_socket
CPPFLAGS.t_socket.c+=	-D_KERNTYPES
LDADD.t_socket+=	-lrumpnet_local -lrumpnet_net -lrumpnet
LDADD.t_socket+=	${LIBRUMPBASE}
.endif

.include <bsd.test.mk>
