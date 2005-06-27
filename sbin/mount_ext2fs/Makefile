#	$NetBSD: Makefile,v 1.7 2005/06/27 01:00:05 christos Exp $
#
#	$OpenBSD: Makefile,v 1.1 1996/06/27 07:20:28 downsj Exp $

.include <bsd.own.mk>

PROG=	mount_ext2fs
SRCS=	mount_ext2fs.c
MAN=	mount_ext2fs.8

DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
