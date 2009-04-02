# $NetBSD: Makefile,v 1.1 2009/04/02 12:58:44 drochner Exp $

WARNS=4

PROG=	findthreads
LDADD=	-lpthread

NOMAN=

regress:
	sh ${.CURDIR}/findthreads.sh

.include <bsd.prog.mk>
