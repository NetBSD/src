#	$NetBSD: Makefile,v 1.2 1999/11/26 18:34:58 msaitoh Exp $

PROG=	cdplay
SRCS=	cdplay.c

LDADD+= -ledit -ltermcap
DPADD+= ${LIBEDIT} ${LIBTERMCAP}

.include <bsd.prog.mk>
