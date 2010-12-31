# $NetBSD: Makefile,v 1.13 2010/12/31 04:08:33 pgoyette Exp $

.include <bsd.own.mk>
.include <bsd.sys.mk>

TESTS_SUBDIRS+=	gen hash setjmp stdlib stdio string

.if ${HAS_SSP} == "yes"
TESTS_SUBDIRS+=	ssp
.endif

.if (${MACHINE_ARCH} != "vax" && ${MACHINE_ARCH} != "m68000")
SUBDIR+= ieeefp
.endif

TESTSDIR=	${TESTSBASE}/lib/libc

TESTS_C+=	t_convfp
TESTS_C+=	t_gdtoa
TESTS_C+=	t_randomid

.include <bsd.test.mk>
