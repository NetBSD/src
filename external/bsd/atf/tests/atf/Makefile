# $NetBSD: Makefile,v 1.5 2010/07/13 21:12:38 jmmv Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/atf
ATFFILE=	yes

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
