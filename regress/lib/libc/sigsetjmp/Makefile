#	$NetBSD: Makefile,v 1.3.2.1 1997/11/08 07:39:42 lukem Exp $

PROG=	sigsetjmptest
SRCS=	jmptest.c
NOMAN=	noman, no way, man

CPPFLAGS+= -DTEST_SIGSETJMP

.PATH:	${.CURDIR}/../setjmp

regress: ${PROG}
	./${PROG} save
	./${PROG} nosave

.include <bsd.prog.mk>
