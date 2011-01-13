# $NetBSD: Makefile,v 1.37 2011/01/13 14:32:35 pgoyette Exp $

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

.include <bsd.test.mk>
