#	$Id: Makefile,v 1.1 1994/12/28 22:12:36 cgd Exp $

PROG=	stdargtest
SRCS=	argtest.c
NOMAN=	noman, no way, man

CFLAGS+= -DSTDARG

.PATH:	${.CURDIR}/../stdarg

install:

regress: ${PROG}
	./${PROG}

.include <bsd.prog.mk>
