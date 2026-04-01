# $NetBSD: Makefile,v 1.53 2026/04/01 21:00:53 rillig Exp $

.include "Makefile.inc"
.include <bsd.own.mk>

SUBDIR+=	tls_dso .WAIT sync

TESTS_SUBDIRS+=	atomic
TESTS_SUBDIRS+=	c063
TESTS_SUBDIRS+=	db
TESTS_SUBDIRS+=	gen
TESTS_SUBDIRS+=	hash
TESTS_SUBDIRS+=	inet
TESTS_SUBDIRS+= kevent_nullmnt
TESTS_SUBDIRS+=	locale
TESTS_SUBDIRS+=	membar
TESTS_SUBDIRS+=	misc
TESTS_SUBDIRS+=	net
TESTS_SUBDIRS+=	nls
TESTS_SUBDIRS+=	regex
TESTS_SUBDIRS+=	rpc
TESTS_SUBDIRS+=	setjmp
TESTS_SUBDIRS+=	stdio
TESTS_SUBDIRS+=	stdlib
TESTS_SUBDIRS+=	string
TESTS_SUBDIRS+=	sys
TESTS_SUBDIRS+=	termios
TESTS_SUBDIRS+=	time
TESTS_SUBDIRS+=	tls
TESTS_SUBDIRS+=	ttyio

.if ${HAVE_SSP} == "yes"
TESTS_SUBDIRS+=	ssp
.endif

TESTSDIR=	${TESTSBASE}/lib/libc

TESTS_C+=	t_cdb
TESTS_C+=	t_convfp
TESTS_C+=	t_gdtoa
TESTS_C+=	t_long_double

.include <bsd.test.mk>
