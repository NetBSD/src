# $NetBSD: Makefile,v 1.10 2011/04/05 06:15:30 jruoho Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/libc/stdlib

TESTS_C+=	t_div
TESTS_C+=	t_environment
TESTS_C+=	t_environment_pth
TESTS_C+=	t_hsearch
TESTS_C+=	t_mi_vector_hash
TESTS_C+=	t_posix_memalign
TESTS_C+=	t_strtod
TESTS_C+=	t_strtox

TESTS_SH+=	t_atexit
TESTS_SH+=	t_getopt

MKMAN=no

BINDIR=		${TESTSDIR}

PROGS+=		h_atexit
PROGS+=		h_getopt h_getopt_long

LDADD.t_environment_pth=	-pthread

.include <bsd.test.mk>
