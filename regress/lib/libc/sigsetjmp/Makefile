#	$NetBSD: Makefile,v 1.5 1999/02/13 02:54:32 lukem Exp $

PROG=	sigsetjmptest
SRCS=	jmptest.c
MKMAN=	no

CPPFLAGS+= -DTEST_SIGSETJMP

.PATH:	${.CURDIR}/../setjmp

regress: ${PROG}
	./${PROG} save
	./${PROG} nosave

.include <bsd.prog.mk>
