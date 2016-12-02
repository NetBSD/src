# $NetBSD: Makefile,v 1.1 2016/12/02 05:54:15 kamil Exp $

NOMAN=		# defined

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/kernel/arch

.if (${MACHINE} == "amd64")
TESTS_SUBDIRS+=	amd64
.endif

.include <bsd.test.mk>
