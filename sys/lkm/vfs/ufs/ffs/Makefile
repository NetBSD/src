#	$NetBSD: Makefile,v 1.2 1996/08/23 05:39:29 mrg Exp $

.include "../Makefile.inc"

.PATH:	$S/ufs/ffs ${.CURDIR}/../..

NOMAN=	yep

KMOD=		ffs
VFS_LKM=	yes

SRCS=	lkminit_vfs.c
SRCS+=	ffs_alloc.c ffs_balloc.c ffs_inode.c ffs_subr.c ffs_tables.c
SRCS+=	ffs_vfsops.c ffs_vnops.c

.include <bsd.kmod.mk>
