# $NetBSD: Makefile,v 1.6.12.2 2014/08/19 23:45:55 tls Exp $

.include <bsd.init.mk>

TESTSDIR=	${TESTSBASE}/atf

TESTS_SUBDIRS=	atf-c \
		atf-c++ \
		atf-sh \
		test-programs

.if !(${MKKYUA} != "no")
TESTS_SUBDIRS+=	tools
.endif

.PATH:		${SRCDIR}

.include <bsd.test.mk>
