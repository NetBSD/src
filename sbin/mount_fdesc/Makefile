#	$NetBSD: Makefile,v 1.13 2005/06/27 01:00:05 christos Exp $
#	@(#)Makefile	8.2 (Berkeley) 3/27/94

.include <bsd.own.mk>

PROG=	mount_fdesc
SRCS=	mount_fdesc.c
MAN=	mount_fdesc.8

DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
