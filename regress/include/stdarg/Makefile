#	$NetBSD: Makefile,v 1.6 2001/12/12 01:24:13 tv Exp $

PROG=		stdargtest
SRCS=		argtest.c
NOMAN=		# defined
CPPFLAGS+=	-DSTDARG

.PATH:		${.CURDIR}/../stdarg

regress: ${PROG}
	./${PROG}

.include <bsd.prog.mk>
