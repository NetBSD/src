#	$NetBSD: Makefile,v 1.3 2002/11/30 03:10:53 lukem Exp $

NOMAN=		# defined

PROG=		getopt

regress: ${PROG} 
	./${PROG} < ${.CURDIR}/opttest

.include <bsd.prog.mk>
