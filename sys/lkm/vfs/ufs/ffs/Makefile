#	$NetBSD: Makefile,v 1.5.22.1 2002/01/10 20:01:29 thorpej Exp $

.include "../Makefile.inc"

.PATH:	$S/ufs/ffs ${.CURDIR}/../..

KMOD=	ffs

SRCS=	lkminit_vfs.c
SRCS+=	ffs_alloc.c ffs_balloc.c ffs_inode.c ffs_subr.c ffs_tables.c
SRCS+=	ffs_vfsops.c ffs_vnops.c

.include <bsd.kmod.mk>
