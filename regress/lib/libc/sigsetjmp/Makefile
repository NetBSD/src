#	$NetBSD: Makefile,v 1.6 2001/12/12 01:24:04 tv Exp $

PROG=		sigsetjmptest
SRCS=		jmptest.c
NOMAN=		# defined
CPPFLAGS+=	-DTEST_SIGSETJMP

.PATH:		${.CURDIR}/../setjmp

regress: ${PROG}
	./${PROG} save
	./${PROG} nosave

.include <bsd.prog.mk>
