#	$NetBSD: Makefile,v 1.5.24.1 2005/04/29 11:29:30 kent Exp $

.include "../Makefile.inc"

.PATH:	$S/ufs/ext2fs ${.CURDIR}/../..

KMOD=	ext2fs

SRCS=	lkminit_vfs.c
SRCS+=	ext2fs_alloc.c ext2fs_balloc.c ext2fs_bmap.c ext2fs_bswap.c
SRCS+=	ext2fs_inode.c ext2fs_lookup.c ext2fs_readwrite.c ext2fs_subr.c
SRCS+=	ext2fs_vfsops.c ext2fs_vnops.c

.include <bsd.kmod.mk>
