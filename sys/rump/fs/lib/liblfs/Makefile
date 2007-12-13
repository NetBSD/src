#	$NetBSD: Makefile,v 1.4 2007/12/13 16:08:12 pooka Exp $
#

.include <bsd.own.mk>

LIB=	lfs

.PATH:  ${NETBSDSRCDIR}/sys/ufs/lfs

SRCS=	lfs_alloc.c lfs_balloc.c lfs_bio.c lfs_cksum.c lfs_debug.c	\
	lfs_inode.c lfs_itimes.c lfs_rfw.c lfs_segment.c lfs_subr.c	\
	lfs_syscalls.c lfs_vfsops.c lfs_vnops.c

CFLAGS+=        -Wno-pointer-sign -DLFS_KERNEL_RFW

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
