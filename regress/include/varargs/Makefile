#	$NetBSD: Makefile,v 1.3.2.1 1997/11/08 07:39:33 lukem Exp $

PROG=	varargstest
SRCS=	argtest.c
NOMAN=	noman, no way, man

CPPFLAGS+= -DVARARGS

.PATH:	${.CURDIR}/../stdarg

regress: ${PROG}
	./${PROG}

.include <bsd.prog.mk>
