#	$NetBSD: Makefile,v 1.1 2006/09/10 11:37:04 kardel Exp $

NOMAN=		# defined

PROG=		timetest

regress: ${PROG}
	./timetest -v -v -c -t 120 -A

.include <bsd.prog.mk>
