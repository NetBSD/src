#	$NetBSD: Makefile,v 1.12 2005/02/05 14:53:03 xtraeme Exp $
#	@(#)Makefile	8.2 (Berkeley) 3/27/94

.include <bsd.own.mk>

PROG=	mount_fdesc
SRCS=	mount_fdesc.c
MAN=	mount_fdesc.8

WARNS=	3

DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
