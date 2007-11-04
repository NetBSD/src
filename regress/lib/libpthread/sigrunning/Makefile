#	$NetBSD: Makefile,v 1.1.2.1 2007/11/04 06:07:52 wrstuden Exp $

WARNS=1
MKDEBUG=1

PROG=   sigrunning
SRCS=   sigrunning.c

LDADD=	-lpthread

NOMAN=

regress:
	./sigrunning

.include <bsd.prog.mk>
