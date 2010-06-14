#	$NetBSD: Makefile,v 1.5 2010/06/14 14:42:49 pooka Exp $
#	@(#)Makefile	8.2 (Berkeley) 3/27/94

.include <bsd.own.mk>

PROG=	mount_ptyfs
SRCS=	mount_ptyfs.c
MAN=	mount_ptyfs.8

DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
