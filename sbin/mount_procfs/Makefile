#	$NetBSD: Makefile,v 1.14 2005/06/27 01:00:06 christos Exp $
#	@(#)Makefile	8.4 (Berkeley) 3/27/94

.include <bsd.own.mk>

PROG=	mount_procfs
SRCS=	mount_procfs.c
MAN=	mount_procfs.8

DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
