#	$Id: Makefile,v 1.1 1994/12/29 00:32:24 cgd Exp $

PROG=	setjmptest
SRCS=	jmptest.c
NOMAN=	noman, no way, man

CFLAGS+= -DTEST_SETJMP

.PATH:	${.CURDIR}/../setjmp

install:

regress: ${PROG}
	./${PROG}

.include <bsd.prog.mk>
