#	$NetBSD: Makefile,v 1.6 2010/02/03 15:34:45 roy Exp $

PROG=	cdplay
SRCS=	cdplay.c

LDADD+= -ledit -lterminfo -lutil
DPADD+= ${LIBEDIT} ${LIBTERMINFO} ${LIBUTIL}

.include <bsd.prog.mk>
