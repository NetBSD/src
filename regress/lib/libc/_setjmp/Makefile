#	$NetBSD: Makefile,v 1.6 2001/12/12 01:24:00 tv Exp $

PROG=		_setjmptest
SRCS=		jmptest.c
NOMAN=		# defined
CPPFLAGS+=	-DTEST_U_SETJMP
.PATH:		${.CURDIR}/../setjmp

regress: ${PROG}
	./${PROG}

.include <bsd.prog.mk>
