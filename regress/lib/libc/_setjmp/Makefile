#	$NetBSD: Makefile,v 1.4 1997/10/26 21:50:59 lukem Exp $

PROG=	_setjmptest
SRCS=	jmptest.c
NOMAN=	noman, no way, man

CPPFLAGS+= -DTEST_U_SETJMP

.PATH:	${.CURDIR}/../setjmp

regress: ${PROG}
	./${PROG}

.include <bsd.prog.mk>
