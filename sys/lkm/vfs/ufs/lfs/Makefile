#	$NetBSD: Makefile,v 1.9 2003/03/15 07:20:22 perseant Exp $

.include "../Makefile.inc"

.PATH:	$S/ufs/lfs $S/ufs/ufs

lfs_vnops.o: ufs_readwrite.c

COPTS+=	-DLFS -Dufs_mkdir=lfs_ufs_mkdir -Dufs_makeinode=lfs_ufs_makeinode \
	-Dufs_strategy=lfs_ufs_strategy -Dufs_whiteout=lfs_ufs_whiteout \
	-Dufs_link=lfs_ufs_link -Dufs_rename=lfs_ufs_rename
KMOD=	lfs

SRCS=	lkminit_vfs.c
SRCS+=	lfs_vfsops.c lfs_vnops.c lfs_subr.c
SRCS+=	lfs_alloc.c lfs_balloc.c lfs_bio.c lfs_cksum.c lfs_debug.c
SRCS+=	lfs_inode.c lfs_segment.c lfs_syscalls.c 
SRCS+=	ufs_vnops.c

.include <bsd.kmod.mk>
