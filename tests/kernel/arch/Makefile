# $NetBSD: Makefile,v 1.2.2.2 2017/01/07 08:56:54 pgoyette Exp $

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
