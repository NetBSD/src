# $NetBSD: Makefile,v 1.1 2010/07/17 19:25:27 jmmv Exp $

NOMAN=		# defined

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/csu

.if exists(arch/${MACHINE_CPU}/initfini_asm.S)
TESTS_SH=	t_crt0

BINDIR=		${TESTSDIR}
PROG=		h_initfini
SRCS=		h_initfini.c initfini_asm.S
WARNS?=		1
.PATH:		${.CURDIR}/arch/${MACHINE_CPU}
.endif

.include <bsd.test.mk>
