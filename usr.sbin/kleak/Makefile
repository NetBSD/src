#	$NetBSD: Makefile,v 1.1.2.2 2018/12/26 14:02:12 pgoyette Exp $

NOMAN=		# defined

PROG=	kleak
SRCS=	kleak.c

LDADD+= -lelf
LDADD+= -lutil
DPADD+= ${LIBELF}
DPADD+= ${LIBUTIL}

.include <bsd.own.mk>
.include <bsd.prog.mk>
