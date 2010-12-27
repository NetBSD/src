# $NetBSD: Makefile,v 1.12 2010/12/27 19:35:30 pgoyette Exp $

.include <bsd.own.mk>
.include <bsd.sys.mk>

TESTS_SUBDIRS+=	gen hash setjmp stdlib stdio string

.if ${HAS_SSP} == "yes"
TESTS_SUBDIRS+=	ssp
.endif

TESTSDIR=	${TESTSBASE}/lib/libc

.include <bsd.test.mk>
