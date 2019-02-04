# $NetBSD: Makefile,v 1.3 2019/02/04 03:10:33 mrg Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/libc/misc

TESTS_C+=	t_ubsan
TESTS_CXX+=	t_ubsanxx

.PATH:		${NETBSDSRCDIR}/common/lib/libc/misc
SRCS.t_ubsan=	t_ubsan.c ubsan.c
SRCS.t_ubsanxx=	t_ubsanxx.cpp ubsan.c

.if ${MKSANITIZER:Uno} != "yes" && ${MKLIBCSANITIZER:Uno} != "yes"
# These tests are designed to be used against micro-UBSan only.
# micro-UBSan is used in these tests as a standalone libary only.
CPPFLAGS+=		-DENABLE_TESTS
UBSAN_FLAGS=		-fsanitize=undefined
UBSAN_FLAGS+=		${${ACTIVE_CC} == "clang" :? -fsanitize=integer :}
UBSAN_FLAGS+=		${${ACTIVE_CC} == "clang" :? -fsanitize=nullability :}
CFLAGS+=		${UBSAN_FLAGS}
CXXFLAGS+=		${UBSAN_FLAGS}
CWARNFLAGS+=		-Wno-return-type -Wno-strict-aliasing
CWARNFLAGS.clang+=	-Wno-incompatible-pointer-types-discards-qualifiers
CWARNFLAGS.clang+=	-Wno-nullability-completeness 
.endif
COPTS.t_ubsan.c += -Wno-stack-protector
COPTS.t_ubsanxx.cpp += -Wno-stack-protector

.if defined(HAVE_GCC) && ${HAVE_GCC} == 7 && ${ACTIVE_CC} == "gcc"
COPTS.t_ubsan.c+=	-Wno-int-in-bool-context
COPTS.t_ubsanxx.cpp+=	-Wno-int-in-bool-context
.endif

.include <bsd.test.mk>
