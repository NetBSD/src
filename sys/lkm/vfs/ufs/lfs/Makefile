#	$NetBSD: Makefile,v 1.4 1999/02/12 05:42:00 cjs Exp $

.include "../Makefile.inc"

.PATH:	$S/ufs/lfs

MKMAN=	no

KMOD=		lfs

SRCS=	lkminit_${KMOD}.c
SRCS+=	lfs_vfsops.c lfs_vnops.c lfs_subr.c
SRCS+=	lfs_alloc.c lfs_balloc.c lfs_bio.c lfs_cksum.c lfs_debug.c
SRCS+=	lfs_inode.c lfs_segment.c lfs_syscalls.c 

.include <bsd.kmod.mk>
