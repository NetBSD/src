#	$NetBSD: Makefile,v 1.2 2015/10/19 16:16:37 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../fs/v7fs

LIB=	rumpfs_v7fs
COMMENT=Unix 7th edition file system

SRCS= v7fs_endian.c v7fs_superblock.c v7fs_inode.c v7fs_dirent.c	\
v7fs_datablock.c v7fs_file.c v7fs_io.c

SRCS+=	v7fs_file_util.c v7fs_inode_util.c v7fs_superblock_util.c

SRCS+=	v7fs_io_kern.c v7fs_extern.c v7fs_vnops.c v7fs_vfsops.c

CPPFLAGS+= -DV7FS_EI

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
