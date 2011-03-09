# $NetBSD: Makefile,v 1.38 2011/03/09 23:10:07 joerg Exp $

.include <bsd.own.mk>
.include <bsd.sys.mk>

SUBDIR+=	tls_dso .WAIT

TESTS_SUBDIRS+=	db gen hash ieeefp inet net regex rpc setjmp stdlib
TESTS_SUBDIRS+=	stdio string sys time tls ttyio

.if ${HAS_SSP} == "yes"
TESTS_SUBDIRS+=	ssp
.endif

TESTSDIR=	${TESTSBASE}/lib/libc

TESTS_C+=	t_convfp
TESTS_C+=	t_gdtoa

.include <bsd.test.mk>
