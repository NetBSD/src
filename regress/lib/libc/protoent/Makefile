#	$NetBSD: Makefile,v 1.2 2004/02/19 19:51:05 christos Exp $

NOMAN=	# defined

PROG=	protoent

regress: ${PROG}
	${.CURDIR}/../servent/compare /etc/protocols $>

.include <bsd.prog.mk>
