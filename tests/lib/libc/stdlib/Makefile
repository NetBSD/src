# $NetBSD: Makefile,v 1.9 2011/01/13 14:32:35 pgoyette Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/libc/stdlib

TESTS_C+=	t_div
TESTS_C+=	t_environment
TESTS_C+=	t_environment_pth
TESTS_C+=	t_hsearch
TESTS_C+=	t_mi_vector_hash
TESTS_C+=	t_posix_memalign
TESTS_C+=	t_strtox

TESTS_SH+=	t_atexit
TESTS_SH+=	t_getopt        

MKMAN=no

BINDIR=		${TESTSDIR}

PROGS+=		h_atexit
PROGS+=		h_getopt h_getopt_long

LDADD.t_environment_pth=	-pthread

.include <bsd.test.mk>
