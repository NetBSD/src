# $NetBSD: Makefile,v 1.2 2011/03/30 09:43:22 jruoho Exp $

NOMAN=		# defined

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/include
TESTS_SUBDIRS=	sys

TESTS_C=	t_bitstring
TESTS_C+=	t_inttypes
TESTS_C+=	t_limits
TESTS_C+=	t_paths
TESTS_C+=	t_stdint

FILESDIR=	${TESTSDIR}
FILES=		d_bitstring_27.out
FILES+=		d_bitstring_32.out
FILES+=		d_bitstring_49.out
FILES+=		d_bitstring_64.out
FILES+=		d_bitstring_67.out
FILES+=		d_bitstring_8.out

.include <bsd.test.mk>
