#	$NetBSD: Makefile,v 1.4 2013/06/16 03:55:35 dholland Exp $

.include "../Makefile.inc"

.PATH:	${S}/ufs/lfs

lfs_vnops.o: ulfs_readwrite.c

KMOD=	lfs
CPPFLAGS+=#-DLFS_DIRHASH -DLFS_EI -DLFS_QUOTA -DLFS_QUOTA2

SRCS=	lfs_vfsops.c lfs_vnops.c lfs_subr.c lfs_alloc.c lfs_balloc.c \
	lfs_bio.c lfs_cksum.c lfs_debug.c lfs_inode.c lfs_segment.c \
	lfs_syscalls.c lfs_itimes.c

SRCS+=	ulfs_bmap.c ulfs_dirhash.c ulfs_ihash.c ulfs_inode.c ulfs_lookup.c \
	ulfs_rename.c ulfs_snapshot.c ulfs_vfsops.c ulfs_vnops.c

.include <bsd.kmodule.mk>
