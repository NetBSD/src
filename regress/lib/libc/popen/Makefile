#	$NetBSD: Makefile,v 1.1.1.1 1999/09/30 09:23:23 tron Exp $

PROG=	popen
SRCS=	popen.c
NOMAN=	noman, no way, man

CLEANFILES+= popen.data

regress: ${PROG}
	./${PROG}

.include <bsd.prog.mk>
