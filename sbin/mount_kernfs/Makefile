#	$NetBSD: Makefile,v 1.13 2005/06/27 01:00:06 christos Exp $
#	@(#)Makefile	8.2 (Berkeley) 3/27/94

.include <bsd.own.mk>

PROG=	mount_kernfs
SRCS=	mount_kernfs.c
MAN=	mount_kernfs.8

DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
