#	$NetBSD: Makefile,v 1.1 2008/01/19 04:12:20 ginsbach Exp $

NOMAN=		# defined

PROG=		inet_network

regress: ${PROG} 
	./${PROG} < ${.CURDIR}/inwtest

.include <bsd.prog.mk>
