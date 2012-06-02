#	$NetBSD: Makefile,v 1.1.42.1 2012/06/02 11:09:37 mrg Exp $

.include "../Makefile.inc"

.PATH:	${S}/ufs/ext2fs

KMOD=	ext2fs
SRCS=	ext2fs_alloc.c ext2fs_balloc.c ext2fs_bmap.c ext2fs_bswap.c \
	ext2fs_inode.c ext2fs_lookup.c ext2fs_readwrite.c ext2fs_rename.c \
	ext2fs_subr.c ext2fs_vfsops.c ext2fs_vnops.c

.include <bsd.kmodule.mk>
