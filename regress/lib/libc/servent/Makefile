#	$NetBSD: Makefile,v 1.2 2004/02/19 19:19:29 christos Exp $

NOMAN=	# defined

PROG=	servent
CPPFLAGS=-D_REENTRANT

regress: ${PROG}
	${.CURDIR}/compare /etc/services $>

.include <bsd.prog.mk>
