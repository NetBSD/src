#	$NetBSD: Makefile,v 1.4 2005/01/12 16:42:45 xtraeme Exp $

WARNS=	3
PROG=	cdplay
SRCS=	cdplay.c

LDADD+= -ledit -ltermcap -lutil
DPADD+= ${LIBEDIT} ${LIBTERMCAP} ${LIBUTIL}

.include <bsd.prog.mk>
