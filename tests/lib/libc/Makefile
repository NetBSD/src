# $NetBSD: Makefile,v 1.41 2011/06/11 18:03:18 christos Exp $

.include "Makefile.inc"
.include <bsd.sys.mk>
.include <bsd.own.mk>

SUBDIR+=	tls_dso .WAIT

TESTS_SUBDIRS+=	db gen hash ieeefp inet locale net regex rpc setjmp stdlib
TESTS_SUBDIRS+=	stdio string sys termios time tls ttyio

.if ${HAS_SSP} == "yes"
TESTS_SUBDIRS+=	ssp
.endif

TESTSDIR=	${TESTSBASE}/lib/libc

TESTS_C+=	t_convfp
TESTS_C+=	t_gdtoa

.include <bsd.test.mk>
