#	$NetBSD: Makefile,v 1.4 1997/10/26 21:50:55 lukem Exp $

PROG=	stdargtest
SRCS=	argtest.c
NOMAN=	noman, no way, man

CPPFLAGS+= -DSTDARG

.PATH:	${.CURDIR}/../stdarg

regress: ${PROG}
	./${PROG}

.include <bsd.prog.mk>
