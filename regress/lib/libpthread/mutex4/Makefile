#	$NetBSD: Makefile,v 1.1 2003/01/30 18:05:27 thorpej Exp $

WARNS=1

PROG=   mutex4
SRCS=   mutex4.c

LDADD=	-lpthread

NOMAN=

regress:
	./mutex4

.include <bsd.prog.mk>
