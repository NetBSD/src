#	$NetBSD: Makefile,v 1.2 2019/02/17 04:06:00 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/v7fs

KMOD=		v7fs
CPPFLAGS+=	-DV7FS_EI

SRCS=	v7fs_endian.c
SRCS+=	v7fs_superblock.c
SRCS+=	v7fs_inode.c
SRCS+=	v7fs_dirent.c
SRCS+=	v7fs_datablock.c
SRCS+=	v7fs_file.c
SRCS+=	v7fs_io.c
SRCS+=	v7fs_file_util.c
SRCS+=	v7fs_inode_util.c
SRCS+=	v7fs_superblock_util.c
SRCS+=	v7fs_io_kern.c
SRCS+=	v7fs_extern.c
SRCS+=	v7fs_vnops.c
SRCS+=	v7fs_vfsops.c

WARNS=	3

.include <bsd.kmodule.mk>
