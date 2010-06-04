# $NetBSD: Makefile,v 1.3 2010/06/04 08:33:41 jmmv Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/atf

SUBDIR=		atf-c \
		atf-c++ \
		atf-check \
		atf-cleanup \
		atf-config \
		atf-report \
		atf-run \
		atf-sh \
		formats \
		test_programs

SRCDIR=		${NETBSDSRCDIR}/external/bsd/atf/dist
.PATH:		${SRCDIR}/tests/atf

.include <bsd.test.mk>
.include <bsd.subdir.mk>
