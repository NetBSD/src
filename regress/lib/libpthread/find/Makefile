# $NetBSD: Makefile,v 1.1.2.2 2009/05/13 19:18:48 jym Exp $

WARNS=4

PROG=	findthreads
LDADD=	-lpthread

NOMAN=

regress:
	sh ${.CURDIR}/findthreads.sh

.include <bsd.prog.mk>
