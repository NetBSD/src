#	$Id: Makefile,v 1.1 1994/12/29 00:32:26 cgd Exp $

PROG=	sigsetjmptest
SRCS=	jmptest.c
NOMAN=	noman, no way, man

CFLAGS+= -DTEST_SIGSETJMP

.PATH:	${.CURDIR}/../setjmp

install:

regress: ${PROG}
	./${PROG} save
	./${PROG} nosave

.include <bsd.prog.mk>
