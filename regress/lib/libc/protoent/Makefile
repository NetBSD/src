#	$NetBSD: Makefile,v 1.3 2004/03/02 14:27:25 drochner Exp $

NOMAN=	# defined

PROG=	protoent

regress: ${PROG}
	${.CURDIR}/../servent/compare /etc/protocols ${.OBJDIR}/${PROG}

.include <bsd.prog.mk>
