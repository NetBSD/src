#	$NetBSD: Makefile,v 1.1 2004/08/03 11:36:23 yamt Exp $

WARNS=3

PROG=   rwlock1
SRCS=   rwlock1.c

LDADD=	-lpthread

NOMAN=

regress:
	./rwlock1

.include <bsd.prog.mk>
