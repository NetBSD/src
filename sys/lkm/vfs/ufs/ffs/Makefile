#	$NetBSD: Makefile,v 1.4 1999/02/12 05:42:00 cjs Exp $

.include "../Makefile.inc"

.PATH:	$S/ufs/ffs ${.CURDIR}/../..

MKMAN=	no

KMOD=		ffs

SRCS=	lkminit_vfs.c
SRCS+=	ffs_alloc.c ffs_balloc.c ffs_inode.c ffs_subr.c ffs_tables.c
SRCS+=	ffs_vfsops.c ffs_vnops.c

.include <bsd.kmod.mk>
