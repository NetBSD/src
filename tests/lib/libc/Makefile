# $NetBSD: Makefile,v 1.39 2011/04/09 17:45:24 pgoyette Exp $

.include <bsd.own.mk>
.include <bsd.sys.mk>

SUBDIR+=	tls_dso .WAIT

TESTS_SUBDIRS+=	db gen hash ieeefp inet locale net regex rpc setjmp stdlib
TESTS_SUBDIRS+=	stdio string sys time tls ttyio

.if ${HAS_SSP} == "yes"
TESTS_SUBDIRS+=	ssp
.endif

TESTSDIR=	${TESTSBASE}/lib/libc

TESTS_C+=	t_convfp
TESTS_C+=	t_gdtoa

.include <bsd.test.mk>
