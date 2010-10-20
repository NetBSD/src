# $NetBSD: Makefile,v 1.6 2010/10/20 09:20:10 jmmv Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/atf
ATFFILE=	yes

SUBDIR=		atf-c \
		atf-c++ \
		atf-config \
		atf-report \
		atf-run \
		atf-sh \
		test-programs

SRCDIR=		${NETBSDSRCDIR}/external/bsd/atf/dist
.PATH:		${SRCDIR}

.include <bsd.test.mk>
