#	$NetBSD: Makefile,v 1.1 2003/01/30 18:05:26 thorpej Exp $

WARNS=1

PROG=   mutex2
SRCS=   mutex2.c

LDADD=	-lpthread

NOMAN=

regress:
	./mutex2

.include <bsd.prog.mk>
