# $NetBSD: Makefile,v 1.33 2011/01/13 01:56:44 pgoyette Exp $

.include <bsd.own.mk>
.include <bsd.sys.mk>

TESTS_SUBDIRS+=	db gen hash ieeefp net regex rpc setjmp stdlib
TESTS_SUBDIRS+=	stdio string sys time ttyio

.if ${HAS_SSP} == "yes"
TESTS_SUBDIRS+=	ssp
.endif

TESTSDIR=	${TESTSBASE}/lib/libc

TESTS_C+=	t_cerror
TESTS_C+=	t_clone
TESTS_C+=	t_context
TESTS_C+=	t_convfp
TESTS_C+=	t_gdtoa
TESTS_C+=	t_hsearch
TESTS_C+=	t_inet
TESTS_C+=	t_ptm
TESTS_C+=	t_randomid

.include <bsd.test.mk>
