#	$NetBSD: Makefile,v 1.11 2003/03/22 12:43:58 jdolecek Exp $
#	@(#)Makefile	8.2 (Berkeley) 3/27/94

.include <bsd.own.mk>

PROG=	mount_fdesc
SRCS=	mount_fdesc.c
MAN=	mount_fdesc.8

DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
