#	$NetBSD: Makefile,v 1.3 1997/05/07 16:01:30 mycroft Exp $

PROG=	stdargtest
SRCS=	argtest.c
NOMAN=	noman, no way, man

CFLAGS+= -DSTDARG

.PATH:	${.CURDIR}/../stdarg

regress: ${PROG}
	./${PROG}

.include <bsd.prog.mk>
