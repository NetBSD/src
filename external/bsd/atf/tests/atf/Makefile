# $NetBSD: Makefile,v 1.4 2010/07/03 08:14:11 jmmv Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/atf

SUBDIR=		atf-c \
		atf-c++ \
		atf-check \
		atf-config \
		atf-report \
		atf-run \
		atf-sh \
		test-programs

SRCDIR=		${NETBSDSRCDIR}/external/bsd/atf/dist
.PATH:		${SRCDIR}

.include <bsd.test.mk>
.include <bsd.subdir.mk>
