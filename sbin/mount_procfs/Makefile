#	$NetBSD: Makefile,v 1.13 2005/02/05 15:14:25 xtraeme Exp $
#	@(#)Makefile	8.4 (Berkeley) 3/27/94

.include <bsd.own.mk>

PROG=	mount_procfs
SRCS=	mount_procfs.c
MAN=	mount_procfs.8

WARNS=	3

DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
