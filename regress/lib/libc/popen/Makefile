#	$NetBSD: Makefile,v 1.1.1.1.4.1 1999/12/27 18:30:19 wrstuden Exp $

PROG=	popen
SRCS=	popen.c
NOMAN=	noman, no way, man

CLEANFILES+= popen.data

regress: ${PROG}
	./${PROG}

.include <bsd.prog.mk>
