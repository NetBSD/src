#	$NetBSD: Makefile,v 1.31 2012/04/07 05:58:55 christos Exp $
#	@(#)Makefile	8.6 (Berkeley) 5/8/95

.include <bsd.own.mk>

PROG=	mount
MAN=	mount.8
SRCS=	mount.c vfslist.c
LDADD+= -lutil
DPADD+= ${LIBUTIL}

.include <bsd.prog.mk>
