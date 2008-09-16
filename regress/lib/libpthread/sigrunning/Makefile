#	$NetBSD: Makefile,v 1.1.10.2 2008/09/16 18:49:33 bouyer Exp $

WARNS=1
MKDEBUG=1

PROG=   sigrunning
SRCS=   sigrunning.c

LDADD=	-lpthread

NOMAN=

regress:
	./sigrunning

.include <bsd.prog.mk>
