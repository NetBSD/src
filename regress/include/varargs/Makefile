#	$NetBSD: Makefile,v 1.4 1997/10/26 21:50:57 lukem Exp $

PROG=	varargstest
SRCS=	argtest.c
NOMAN=	noman, no way, man

CPPFLAGS+= -DVARARGS

.PATH:	${.CURDIR}/../stdarg

regress: ${PROG}
	./${PROG}

.include <bsd.prog.mk>
