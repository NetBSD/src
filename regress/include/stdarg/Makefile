#	$NetBSD: Makefile,v 1.3.2.1 1997/11/08 07:39:30 lukem Exp $

PROG=	stdargtest
SRCS=	argtest.c
NOMAN=	noman, no way, man

CPPFLAGS+= -DSTDARG

.PATH:	${.CURDIR}/../stdarg

regress: ${PROG}
	./${PROG}

.include <bsd.prog.mk>
