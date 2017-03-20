# $NetBSD: Makefile,v 1.2.2.3 2017/03/20 06:57:57 pgoyette Exp $

NOMAN=		# defined

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/kernel/arch

.if (${MACHINE} == "amd64")
TESTS_SUBDIRS+=	amd64
.endif

.if (${MACHINE} == "i386")
TESTS_SUBDIRS+=	i386
.endif

.if ${MACHINE_ARCH} == "x86_64" || ${MACHINE_ARCH} == "i386"
TESTS_SUBDIRS+= x86
.endif

.include <bsd.test.mk>
