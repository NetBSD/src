#	$NetBSD: Makefile,v 1.1 2004/02/19 19:18:48 christos Exp $

NOMAN=	# defined

PROG=	protoent
CPPFLAGS=-D_REENTRANT

regress: ${PROG}
	${.CURDIR}/../servent/compare /etc/protocols $>

.include <bsd.prog.mk>
