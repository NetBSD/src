#	$NetBSD: Makefile,v 1.1 2002/09/20 21:13:29 christos Exp $

PROG=		snprintbtest
NOMAN=		# defined

regress: ${PROG}
	./${PROG}

DPADD+= ${LIBUTIL}
LDADD+= -lutil

.include <bsd.prog.mk>
