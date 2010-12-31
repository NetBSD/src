# $NetBSD: Makefile,v 1.15 2010/12/31 14:36:11 pgoyette Exp $

.include <bsd.own.mk>
.include <bsd.sys.mk>

TESTS_SUBDIRS+=	gen hash setjmp stdlib stdio string

.if ${HAS_SSP} == "yes"
TESTS_SUBDIRS+=	ssp
.endif

TESTSDIR=	${TESTSBASE}/lib/libc

TESTS_C+=	t_clone
TESTS_C+=	t_context
TESTS_C+=	t_convfp
TESTS_C+=	t_gdtoa
TESTS_C+=	t_hsearch
TESTS_C+=	t_inet
TESTS_C+=	t_ptm
TESTS_C+=	t_randomid
TESTS_C+=	t_strptime

.include <bsd.test.mk>
