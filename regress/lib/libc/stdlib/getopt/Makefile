#	$NetBSD: Makefile,v 1.2 2002/09/18 05:41:41 lukem Exp $

NOMAN=		# defined

PROG=		getopt

regress: ${PROG} 
	./${PROG} < ${.CURDIR}/opttest

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.prog.mk>
