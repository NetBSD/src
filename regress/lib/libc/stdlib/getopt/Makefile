#	$NetBSD: Makefile,v 1.1 2002/07/15 22:04:00 christos Exp $

PROG=		getopt
NOMAN=		# defined

regress: ${PROG} 
	./${PROG} < ${.CURDIR}/opttest

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.prog.mk>
