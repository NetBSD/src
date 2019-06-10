#	$NetBSD: Makefile,v 1.1.4.2 2019/06/10 22:10:32 christos Exp $

NOMAN=		# defined

PROG=	kleak
SRCS=	kleak.c

LDADD+= -lelf
LDADD+= -lutil
DPADD+= ${LIBELF}
DPADD+= ${LIBUTIL}

.include <bsd.own.mk>
.include <bsd.prog.mk>
