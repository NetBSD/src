#	$NetBSD: Makefile,v 1.3 2002/09/18 05:41:50 lukem Exp $

NOMAN=		# defined

PROG=		sigstackalign
SRCS=		sigstackalign.c stackptr.S

regress: ${PROG}
	./${PROG}

.include <bsd.prog.mk>
