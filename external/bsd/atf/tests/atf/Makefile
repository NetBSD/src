# $NetBSD: Makefile,v 1.6.12.1 2013/06/23 06:26:23 tls Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/atf

TESTS_SUBDIRS=	atf-c \
		atf-c++ \
		atf-sh \
		test-programs

.if !(${MKKYUA} != "no")
TESTS_SUBDIRS+=	atf-config \
		atf-report \
		atf-run
.endif

SRCDIR=		${NETBSDSRCDIR}/external/bsd/atf/dist
.PATH:		${SRCDIR}

.include <bsd.test.mk>
