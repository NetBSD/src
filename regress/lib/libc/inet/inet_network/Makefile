#	$NetBSD: Makefile,v 1.1.4.2 2008/03/23 00:42:58 matt Exp $

NOMAN=		# defined

PROG=		inet_network

regress: ${PROG} 
	./${PROG} < ${.CURDIR}/inwtest

.include <bsd.prog.mk>
