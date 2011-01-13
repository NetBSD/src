# $NetBSD: Makefile,v 1.36 2011/01/13 13:53:49 pgoyette Exp $

.include <bsd.own.mk>
.include <bsd.sys.mk>

TESTS_SUBDIRS+=	db gen hash ieeefp inet net regex rpc setjmp stdlib
TESTS_SUBDIRS+=	stdio string sys time ttyio

.if ${HAS_SSP} == "yes"
TESTS_SUBDIRS+=	ssp
.endif

TESTSDIR=	${TESTSBASE}/lib/libc

TESTS_C+=	t_convfp
TESTS_C+=	t_gdtoa
TESTS_C+=	t_hsearch

.include <bsd.test.mk>
