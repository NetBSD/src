# $NetBSD: Makefile,v 1.6.6.1 2014/05/22 15:40:14 yamt Exp $

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
