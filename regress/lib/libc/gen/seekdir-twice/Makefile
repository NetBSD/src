#	$NetBSD: Makefile,v 1.1 2006/05/17 21:12:23 christos Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=	seekdir-twice

regress: ${PROG}
	./${PROG}

CLEANFILES+=t/a t/b t/c

.include <bsd.prog.mk>
