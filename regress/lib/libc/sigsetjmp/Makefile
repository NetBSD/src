#	$NetBSD: Makefile,v 1.4 1997/10/26 21:51:05 lukem Exp $

PROG=	sigsetjmptest
SRCS=	jmptest.c
NOMAN=	noman, no way, man

CPPFLAGS+= -DTEST_SIGSETJMP

.PATH:	${.CURDIR}/../setjmp

regress: ${PROG}
	./${PROG} save
	./${PROG} nosave

.include <bsd.prog.mk>
