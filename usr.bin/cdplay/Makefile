#	$NetBSD: Makefile,v 1.1.4.1 1999/12/27 18:36:47 wrstuden Exp $

PROG=	cdplay
SRCS=	cdplay.c

LDADD+= -ledit -ltermcap
DPADD+= ${LIBEDIT} ${LIBTERMCAP}

.include <bsd.prog.mk>
