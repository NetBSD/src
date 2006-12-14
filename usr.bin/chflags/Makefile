#	$NetBSD: Makefile,v 1.8 2006/12/14 20:57:00 he Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>

WARNS=	3
PROG=	chflags
SRCS=	chflags.c

LDADD+= -lutil
DPADD+= ${LIBUTIL}

.include <bsd.prog.mk>
