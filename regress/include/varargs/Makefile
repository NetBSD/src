#	$NetBSD: Makefile,v 1.7 2002/09/18 05:41:36 lukem Exp $

NOMAN=		# defined

PROG=		varargstest
SRCS=		argtest.c
CPPFLAGS+=	-DVARARGS

.PATH:		${.CURDIR}/../stdarg

regress: ${PROG}
	./${PROG}

.include <bsd.prog.mk>
