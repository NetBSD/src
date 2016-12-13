# $NetBSD: Makefile,v 1.2 2016/12/13 18:00:10 kamil Exp $

NOMAN=		# defined

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/kernel/arch

.if (${MACHINE} == "amd64")
TESTS_SUBDIRS+=	amd64
.endif

.if (${MACHINE} == "i386")
TESTS_SUBDIRS+=	i386
.endif

.include <bsd.test.mk>
