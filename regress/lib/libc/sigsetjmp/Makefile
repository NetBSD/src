#	$NetBSD: Makefile,v 1.3 1997/05/07 16:01:43 mycroft Exp $

PROG=	sigsetjmptest
SRCS=	jmptest.c
NOMAN=	noman, no way, man

CFLAGS+= -DTEST_SIGSETJMP

.PATH:	${.CURDIR}/../setjmp

regress: ${PROG}
	./${PROG} save
	./${PROG} nosave

.include <bsd.prog.mk>
