#	$NetBSD: Makefile,v 1.6 2005/02/05 14:51:16 xtraeme Exp $
#
#	$OpenBSD: Makefile,v 1.1 1996/06/27 07:20:28 downsj Exp $

.include <bsd.own.mk>

PROG=	mount_ext2fs
SRCS=	mount_ext2fs.c
MAN=	mount_ext2fs.8

WARNS=	3

DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
