#	$NetBSD: Makefile,v 1.1 2001/08/06 22:29:59 bjh21 Exp $

PROG=	sigstackalign
SRCS=	sigstackalign.c stackptr.S
MKMAN=	no

regress: ${PROG}
	./${PROG}

.include <bsd.prog.mk>