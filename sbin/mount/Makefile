#	$NetBSD: Makefile,v 1.32 2016/01/23 21:22:47 christos Exp $
#	@(#)Makefile	8.6 (Berkeley) 5/8/95

.include <bsd.own.mk>

CPPFLAGS+=	-D_KERNTYPES
PROG=	mount
MAN=	mount.8
SRCS=	mount.c vfslist.c
LDADD+= -lutil
DPADD+= ${LIBUTIL}

.include <bsd.prog.mk>
