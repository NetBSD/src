#	$NetBSD: Makefile,v 1.6 2001/12/12 01:50:18 tv Exp $

.include "../Makefile.inc"

.PATH:	$S/ufs/ffs ${.CURDIR}/../..

NOMAN=	# defined

KMOD=	ffs

SRCS=	lkminit_vfs.c
SRCS+=	ffs_alloc.c ffs_balloc.c ffs_inode.c ffs_subr.c ffs_tables.c
SRCS+=	ffs_vfsops.c ffs_vnops.c

.include <bsd.kmod.mk>
