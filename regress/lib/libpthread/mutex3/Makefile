#	$NetBSD: Makefile,v 1.1 2003/01/30 18:05:26 thorpej Exp $

WARNS=1

PROG=   mutex3
SRCS=   mutex3.c

LDADD=	-lpthread

NOMAN=

regress:
	./mutex3

.include <bsd.prog.mk>
