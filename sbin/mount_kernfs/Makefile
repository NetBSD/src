#	$NetBSD: Makefile,v 1.12 2005/02/05 15:01:09 xtraeme Exp $
#	@(#)Makefile	8.2 (Berkeley) 3/27/94

.include <bsd.own.mk>

PROG=	mount_kernfs
SRCS=	mount_kernfs.c
MAN=	mount_kernfs.8

WARNS=	3

DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
