#	$Id: Makefile,v 1.1 1994/12/29 00:32:22 cgd Exp $

PROG=	_setjmptest
SRCS=	jmptest.c
NOMAN=	noman, no way, man

CFLAGS+= -DTEST_U_SETJMP

.PATH:	${.CURDIR}/../setjmp

install:

regress: ${PROG}
	./${PROG}

.include <bsd.prog.mk>
