#	$NetBSD: Makefile,v 1.1 2004/02/19 19:06:52 christos Exp $

NOMAN=	# defined

PROG=	servent
CPPFLAGS=-D_REENTRANT

regress: ${PROG}
	${.CURDIR}/compare $>

.include <bsd.prog.mk>
