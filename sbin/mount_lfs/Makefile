#	$NetBSD: Makefile,v 1.8 2003/03/22 12:43:59 jdolecek Exp $
#	@(#)Makefile	8.2 (Berkeley) 3/27/94

.include <bsd.own.mk>

PROG=	mount_lfs
SRCS=	mount_lfs.c
MAN=	mount_lfs.8

DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
