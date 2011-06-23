# $NetBSD: Makefile,v 1.12.2.1 2011/06/23 14:20:40 cherry Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/libc/stdlib

TESTS_C+=	t_div
TESTS_C+=	t_environment
TESTS_C+=	t_environment_pth
TESTS_C+=	t_exit
TESTS_C+=	t_hsearch
TESTS_C+=	t_mi_vector_hash
TESTS_C+=	t_posix_memalign
TESTS_C+=	t_strtod
TESTS_C+=	t_strtol

TESTS_SH+=	t_atexit
TESTS_SH+=	t_getopt

MKMAN=no

BINDIR=		${TESTSDIR}

PROGS+=		h_atexit
PROGS+=		h_getopt h_getopt_long

LDADD.t_strtod=			-lm
LDADD.t_environment_pth=	-pthread

COPTS+=				-ffloat-store

.include <bsd.test.mk>
