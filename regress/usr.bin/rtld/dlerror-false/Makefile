# $NetBSD: Makefile,v 1.1 2002/11/14 21:10:47 nathanw Exp $

NOMAN=	# defined

PROG=	dlerror-false

LDADD=	-Wl,-rpath,/var/nonexistent/lib

regress:
	./${PROG}

.include <bsd.prog.mk>