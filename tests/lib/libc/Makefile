# $NetBSD: Makefile,v 1.47.26.2 2020/04/08 14:09:08 martin Exp $

.include "Makefile.inc"
.include <bsd.own.mk>

SUBDIR+=	tls_dso .WAIT sync

TESTS_SUBDIRS+=	atomic
TESTS_SUBDIRS+=	c063 db gen hash inet locale misc net nls regex rpc setjmp
TESTS_SUBDIRS+=	stdlib stdio string sys termios time tls ttyio

.if ${HAVE_SSP} == "yes"
TESTS_SUBDIRS+=	ssp
.endif

TESTSDIR=	${TESTSBASE}/lib/libc

TESTS_C+=	t_convfp
TESTS_C+=	t_gdtoa
TESTS_C+=	t_cdb

.include <bsd.test.mk>
