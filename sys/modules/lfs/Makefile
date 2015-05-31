#	$NetBSD: Makefile,v 1.8 2015/05/31 15:48:02 hannken Exp $

.include "../Makefile.inc"

.PATH:	${S}/ufs/lfs

lfs_vnops.o: ulfs_readwrite.c

KMOD=	lfs
CPPFLAGS+=#-DLFS_DIRHASH -DLFS_EI -DLFS_QUOTA -DLFS_QUOTA2

SRCS=	lfs_vfsops.c lfs_vnops.c lfs_subr.c lfs_alloc.c lfs_balloc.c \
	lfs_bio.c lfs_cksum.c lfs_debug.c lfs_inode.c lfs_pages.c \
	lfs_segment.c lfs_rename.c lfs_syscalls.c lfs_itimes.c

SRCS+=	ulfs_bmap.c ulfs_dirhash.c ulfs_inode.c ulfs_lookup.c \
	ulfs_snapshot.c ulfs_vfsops.c ulfs_vnops.c

.include <bsd.kmodule.mk>
