#	$NetBSD: Makefile,v 1.1 2018/12/02 21:00:13 maxv Exp $

NOMAN=		# defined

PROG=	kleak
SRCS=	kleak.c

LDADD+= -lelf
LDADD+= -lutil
DPADD+= ${LIBELF}
DPADD+= ${LIBUTIL}

.include <bsd.own.mk>
.include <bsd.prog.mk>
