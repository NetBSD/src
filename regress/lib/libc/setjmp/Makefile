#	$NetBSD: Makefile,v 1.3.2.1 1997/11/08 07:39:40 lukem Exp $

PROG=	setjmptest
SRCS=	jmptest.c
NOMAN=	noman, no way, man

CPPFLAGS+= -DTEST_SETJMP

.PATH:	${.CURDIR}/../setjmp

regress: ${PROG}
	./${PROG}

.include <bsd.prog.mk>
