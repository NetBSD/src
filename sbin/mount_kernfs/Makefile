#	$NetBSD: Makefile,v 1.11 2003/03/22 12:43:59 jdolecek Exp $
#	@(#)Makefile	8.2 (Berkeley) 3/27/94

.include <bsd.own.mk>

PROG=	mount_kernfs
SRCS=	mount_kernfs.c
MAN=	mount_kernfs.8

DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
