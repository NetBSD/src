#	$NetBSD: Makefile,v 1.1 2003/01/30 18:05:25 thorpej Exp $

WARNS=1

PROG=   mutex1
SRCS=   mutex1.c

LDADD=	-lpthread

NOMAN=

regress:
	./mutex1

.include <bsd.prog.mk>
