#	$NetBSD: Makefile,v 1.3 2000/12/16 01:36:00 lukem Exp $

PROG=	cdplay
SRCS=	cdplay.c

LDADD+= -ledit -ltermcap -lutil
DPADD+= ${LIBEDIT} ${LIBTERMCAP} ${LIBUTIL}

.include <bsd.prog.mk>
