#	$NetBSD: Makefile,v 1.4.2.1 2017/12/03 11:39:11 jdolecek Exp $
#

.PATH:  ${.CURDIR}/../../../../ufs/ext2fs

LIB=	rumpfs_ext2fs
COMMENT=Linux Ext2

SRCS=	ext2fs_alloc.c ext2fs_balloc.c ext2fs_bmap.c ext2fs_bswap.c	\
	ext2fs_hash.c ext2fs_htree.c \
	ext2fs_extents.c ext2fs_inode.c ext2fs_lookup.c ext2fs_readwrite.c \
	ext2fs_rename.c ext2fs_subr.c ext2fs_vfsops.c ext2fs_vnops.c \
	ext2fs_xattr.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
