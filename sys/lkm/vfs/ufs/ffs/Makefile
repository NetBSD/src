#	$NetBSD: Makefile,v 1.5.20.1 2002/01/08 00:33:29 nathanw Exp $

.include "../Makefile.inc"

.PATH:	$S/ufs/ffs ${.CURDIR}/../..

KMOD=	ffs

SRCS=	lkminit_vfs.c
SRCS+=	ffs_alloc.c ffs_balloc.c ffs_inode.c ffs_subr.c ffs_tables.c
SRCS+=	ffs_vfsops.c ffs_vnops.c

.include <bsd.kmod.mk>
