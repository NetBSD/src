#	$NetBSD: Makefile,v 1.1 2003/01/30 18:23:09 thorpej Exp $

WARNS=1

PROG=   barrier1
SRCS=   barrier1.c

LDADD=	-lpthread

NOMAN=

regress:
	./barrier1

.include <bsd.prog.mk>
