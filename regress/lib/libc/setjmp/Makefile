#	$NetBSD: Makefile,v 1.7 2002/09/18 05:41:41 lukem Exp $

NOMAN=		# defined

PROG=		setjmptest
SRCS=		jmptest.c
CPPFLAGS+=	-DTEST_SETJMP

.PATH:		${.CURDIR}/../setjmp

regress: ${PROG}
	./${PROG}

.include <bsd.prog.mk>
