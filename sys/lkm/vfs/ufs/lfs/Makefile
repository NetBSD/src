#	$NetBSD: Makefile,v 1.7 2001/12/12 01:50:19 tv Exp $

.include "../Makefile.inc"

.PATH:	$S/ufs/lfs

NOMAN=	# defined

KMOD=	lfs

SRCS=	lkminit_vfs.c
SRCS+=	lfs_vfsops.c lfs_vnops.c lfs_subr.c
SRCS+=	lfs_alloc.c lfs_balloc.c lfs_bio.c lfs_cksum.c lfs_debug.c
SRCS+=	lfs_inode.c lfs_segment.c lfs_syscalls.c 

.include <bsd.kmod.mk>
