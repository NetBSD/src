# $NetBSD: Makefile,v 1.44 2012/11/18 17:41:54 manu Exp $

.include "Makefile.inc"
.include <bsd.sys.mk>
.include <bsd.own.mk>

SUBDIR+=	tls_dso .WAIT

TESTS_SUBDIRS+=	c063 db gen hash inet locale net regex rpc setjmp stdlib
TESTS_SUBDIRS+=	stdio string sys termios time tls ttyio

.if ${HAS_SSP} == "yes"
TESTS_SUBDIRS+=	ssp
.endif

TESTSDIR=	${TESTSBASE}/lib/libc

TESTS_C+=	t_convfp
TESTS_C+=	t_gdtoa
TESTS_C+=	t_cdb

.include <bsd.test.mk>
