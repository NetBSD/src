#	$NetBSD: Makefile,v 1.4 2004/03/02 14:27:25 drochner Exp $

NOMAN=	# defined

PROG=	servent

regress: ${PROG}
	${.CURDIR}/compare /etc/services ${.OBJDIR}/${PROG}

.include <bsd.prog.mk>
