#	$NetBSD: Makefile,v 1.7 2002/09/18 05:41:36 lukem Exp $

NOMAN=		# defined

PROG=		stdargtest
SRCS=		argtest.c
CPPFLAGS+=	-DSTDARG

.PATH:		${.CURDIR}/../stdarg

regress: ${PROG}
	./${PROG}

.include <bsd.prog.mk>
