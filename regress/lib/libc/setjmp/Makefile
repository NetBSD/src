#	$NetBSD: Makefile,v 1.3 1997/05/07 16:01:42 mycroft Exp $

PROG=	setjmptest
SRCS=	jmptest.c
NOMAN=	noman, no way, man

CFLAGS+= -DTEST_SETJMP

.PATH:	${.CURDIR}/../setjmp

regress: ${PROG}
	./${PROG}

.include <bsd.prog.mk>
