# $NetBSD: Makefile,v 1.34 2023/07/04 15:06:36 riastradh Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/libc/stdlib

TESTS_C+=	t_a64l
TESTS_C+=	t_abs
TESTS_C+=	t_atoi
TESTS_C+=	t_div
TESTS_C+=	t_getenv
TESTS_C+=	t_getenv_thread
TESTS_C+=	t_exit
TESTS_C+=	t_hsearch
TESTS_C+=	t_mktemp
TESTS_C+=	t_mi_vector_hash
TESTS_C+=	t_posix_memalign
TESTS_C+=	t_random
TESTS_C+=	t_strtod
TESTS_C+=	t_strtol
TESTS_C+=	t_strtoi
TESTS_C+=	t_system

TESTS_SH+=	t_atexit
TESTS_SH+=	t_getopt

MKMAN=no

BINDIR=		${TESTSDIR}

PROGS+=		h_atexit
PROGS+=		h_getopt h_getopt_long

CFLAGS.t_posix_memalign.c+=	-fno-builtin-posix_memalign
CFLAGS.t_posix_memalign.c+=	-fno-builtin-aligned_alloc

CPPFLAGS.t_strtod.c+=  -D__TEST_FENV
LDADD.t_strtod=			-lm
DPADD.t_strtod+=		${LIBM}

LDADD.t_getenv_thread=	-lpthread
LDADD.t_mktemp+=	-Wl,--no-fatal-warnings

.include <bsd.test.mk>
