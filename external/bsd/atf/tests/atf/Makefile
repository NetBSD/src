# $NetBSD: Makefile,v 1.2 2010/05/08 08:12:34 jmmv Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/atf

SUBDIR=		atf-c \
		atf-c++ \
		atf-check \
		atf-cleanup \
		atf-compile \
		atf-config \
		atf-report \
		atf-run \
		atf-sh \
		data \
		formats \
		test_programs

SRCDIR=		${NETBSDSRCDIR}/external/bsd/atf/dist
.PATH:		${SRCDIR}/tests/atf

.include <bsd.test.mk>
.include <bsd.subdir.mk>
