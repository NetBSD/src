#	$Id: Makefile,v 1.1 1994/12/28 22:12:39 cgd Exp $

PROG=	varargstest
SRCS=	argtest.c
NOMAN=	noman, no way, man

CFLAGS+= -DVARARGS

.PATH:	${.CURDIR}/../stdarg

install:

regress: ${PROG}
	./${PROG}

.include <bsd.prog.mk>
