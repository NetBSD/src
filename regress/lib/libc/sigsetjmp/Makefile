#	$NetBSD: Makefile,v 1.7 2002/09/18 05:41:41 lukem Exp $

NOMAN=		# defined

PROG=		sigsetjmptest
SRCS=		jmptest.c
CPPFLAGS+=	-DTEST_SIGSETJMP

.PATH:		${.CURDIR}/../setjmp

regress: ${PROG}
	./${PROG} save
	./${PROG} nosave

.include <bsd.prog.mk>
