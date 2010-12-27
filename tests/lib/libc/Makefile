# $NetBSD: Makefile,v 1.11 2010/12/27 15:10:33 he Exp $

.include <bsd.own.mk>
.include <bsd.sys.mk>

TESTS_SUBDIRS+=	gen hash stdlib stdio string

.if ${HAS_SSP} == "yes"
TESTS_SUBDIRS+=	ssp
.endif

TESTSDIR=	${TESTSBASE}/lib/libc

.include <bsd.test.mk>
