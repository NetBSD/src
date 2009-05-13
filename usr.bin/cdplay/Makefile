#	$NetBSD: Makefile,v 1.4.36.1 2009/05/13 19:19:45 jym Exp $

PROG=	cdplay
SRCS=	cdplay.c

LDADD+= -ledit -ltermcap -lutil
DPADD+= ${LIBEDIT} ${LIBTERMCAP} ${LIBUTIL}

.include <bsd.prog.mk>
