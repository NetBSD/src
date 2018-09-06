# $NetBSD: Makefile,v 1.47.24.1 2018/09/06 06:56:48 pgoyette Exp $

.include "Makefile.inc"
.include <bsd.own.mk>

SUBDIR+=	tls_dso .WAIT sync

TESTS_SUBDIRS+=	c063 db gen hash inet locale misc net regex rpc setjmp stdlib
TESTS_SUBDIRS+=	stdio string sys termios time tls ttyio

.if ${HAVE_SSP} == "yes"
TESTS_SUBDIRS+=	ssp
.endif

TESTSDIR=	${TESTSBASE}/lib/libc

TESTS_C+=	t_convfp
TESTS_C+=	t_gdtoa
TESTS_C+=	t_cdb

.include <bsd.test.mk>
