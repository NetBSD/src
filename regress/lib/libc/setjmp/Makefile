#	$NetBSD: Makefile,v 1.4 1997/10/26 21:51:04 lukem Exp $

PROG=	setjmptest
SRCS=	jmptest.c
NOMAN=	noman, no way, man

CPPFLAGS+= -DTEST_SETJMP

.PATH:	${.CURDIR}/../setjmp

regress: ${PROG}
	./${PROG}

.include <bsd.prog.mk>
