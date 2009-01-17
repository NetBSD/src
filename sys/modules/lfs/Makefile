#	$NetBSD: Makefile,v 1.1.2.3 2009/01/17 13:29:30 mjf Exp $

.include "../Makefile.inc"

.PATH:	${S}/ufs/lfs ${S}/ufs/ufs

lfs_vnops.o: ufs_readwrite.c

COPTS+=	-DLFS -Dufs_mkdir=lfs_ufs_mkdir -Dufs_makeinode=lfs_ufs_makeinode \
	-Dufs_strategy=lfs_ufs_strategy -Dufs_whiteout=lfs_ufs_whiteout \
	-Dufs_link=lfs_ufs_link -Dufs_rename=lfs_ufs_rename

KMOD=	lfs
SRCS=	lfs_vfsops.c lfs_vnops.c lfs_subr.c lfs_alloc.c lfs_balloc.c \
	lfs_bio.c lfs_cksum.c lfs_debug.c lfs_inode.c lfs_segment.c \
	lfs_syscalls.c ufs_vnops.c lfs_itimes.c

.include <bsd.kmodule.mk>
