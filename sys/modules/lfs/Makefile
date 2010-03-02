#	$NetBSD: Makefile,v 1.3 2010/03/02 18:00:24 pooka Exp $

.include "../Makefile.inc"

.PATH:	${S}/ufs/lfs ${S}/ufs/ufs

lfs_vnops.o: ufs_readwrite.c

KMOD=	lfs
SRCS=	lfs_vfsops.c lfs_vnops.c lfs_subr.c lfs_alloc.c lfs_balloc.c \
	lfs_bio.c lfs_cksum.c lfs_debug.c lfs_inode.c lfs_segment.c \
	lfs_syscalls.c lfs_itimes.c

.include <bsd.kmodule.mk>
