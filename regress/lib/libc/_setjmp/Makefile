#	$NetBSD: Makefile,v 1.7 2002/09/18 05:41:36 lukem Exp $

NOMAN=		# defined

PROG=		_setjmptest
SRCS=		jmptest.c
CPPFLAGS+=	-DTEST_U_SETJMP
.PATH:		${.CURDIR}/../setjmp

regress: ${PROG}
	./${PROG}

.include <bsd.prog.mk>
