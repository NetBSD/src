#	$NetBSD: Makefile,v 1.5 1999/02/13 02:54:31 lukem Exp $

PROG=	stdargtest
SRCS=	argtest.c
MKMAN=	no

CPPFLAGS+= -DSTDARG

.PATH:	${.CURDIR}/../stdarg

regress: ${PROG}
	./${PROG}

.include <bsd.prog.mk>
