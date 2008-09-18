#	$NetBSD: Makefile,v 1.4.12.1 2008/09/18 04:37:03 wrstuden Exp $
#

.PATH:  ${.CURDIR}/../../../../ufs/lfs

LIB=	rumpfs_lfs

SRCS=	lfs_alloc.c lfs_balloc.c lfs_bio.c lfs_cksum.c lfs_debug.c	\
	lfs_inode.c lfs_itimes.c lfs_rfw.c lfs_segment.c lfs_subr.c	\
	lfs_syscalls.c lfs_vfsops.c lfs_vnops.c

CFLAGS+=        -Wno-pointer-sign -DLFS_KERNEL_RFW

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
