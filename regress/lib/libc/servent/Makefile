#	$NetBSD: Makefile,v 1.3 2004/02/19 19:39:13 christos Exp $

NOMAN=	# defined

PROG=	servent

regress: ${PROG}
	${.CURDIR}/compare /etc/services $>

.include <bsd.prog.mk>
