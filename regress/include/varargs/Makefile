#	$NetBSD: Makefile,v 1.3 1997/05/07 16:01:31 mycroft Exp $

PROG=	varargstest
SRCS=	argtest.c
NOMAN=	noman, no way, man

CFLAGS+= -DVARARGS

.PATH:	${.CURDIR}/../stdarg

regress: ${PROG}
	./${PROG}

.include <bsd.prog.mk>
