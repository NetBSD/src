#	$NetBSD: Makefile,v 1.6 2001/12/12 01:24:03 tv Exp $

PROG=		setjmptest
SRCS=		jmptest.c
NOMAN=		# defined
CPPFLAGS+=	-DTEST_SETJMP

.PATH:		${.CURDIR}/../setjmp

regress: ${PROG}
	./${PROG}

.include <bsd.prog.mk>
