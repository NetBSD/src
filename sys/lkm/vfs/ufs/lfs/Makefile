#	$NetBSD: Makefile,v 1.2 1996/08/23 05:39:31 mrg Exp $

.include "../Makefile.inc"

.PATH:	$S/ufs/lfs

NOMAN=	yep

KMOD=		lfs
VFS_LKM=	yes

SRCS=	lkminit_${KMOD}.c
SRCS+=	lfs_vfsops.c lfs_vnops.c lfs_subr.c
SRCS+=	lfs_alloc.c lfs_balloc.c lfs_bio.c lfs_cksum.c lfs_debug.c
SRCS+=	lfs_inode.c lfs_segment.c lfs_syscalls.c 

.include <bsd.kmod.mk>
