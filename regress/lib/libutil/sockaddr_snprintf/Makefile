#	$NetBSD: Makefile,v 1.1 2004/11/19 21:38:25 christos Exp $

PROG=		sockaddr_snprintf
NOMAN=		# defined

regress: ${PROG}
	./${PROG}

DPADD+= ${LIBUTIL}
LDADD+= -lutil

.include <bsd.prog.mk>
