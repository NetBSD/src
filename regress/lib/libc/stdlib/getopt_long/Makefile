#	$NetBSD: Makefile,v 1.1 2007/01/18 16:29:21 ginsbach Exp $

NOMAN=		# defined

PROG=		getopt_long

regress: ${PROG} 
	./${PROG} < ${.CURDIR}/opttest

.include <bsd.prog.mk>
