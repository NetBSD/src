#	$NetBSD: Makefile,v 1.5 1999/02/13 02:54:32 lukem Exp $

PROG=	setjmptest
SRCS=	jmptest.c
MKMAN=	no

CPPFLAGS+= -DTEST_SETJMP

.PATH:	${.CURDIR}/../setjmp

regress: ${PROG}
	./${PROG}

.include <bsd.prog.mk>
