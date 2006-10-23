#	$NetBSD: Makefile,v 1.1 2006/10/23 00:44:53 pooka Exp $

PROG= dtfs
SRCS= dtfs.c dtfs_vfsops.c dtfs_vnops.c dtfs_subr.c
LDADD+= -lpuffs -lutil
DBG=-g -O0

WARNS=2

NOMAN= has gone before

.include <bsd.prog.mk>
