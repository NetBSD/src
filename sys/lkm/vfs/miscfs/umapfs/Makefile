#	$NetBSD: Makefile,v 1.1 1996/08/22 20:18:56 explorer Exp $

.PATH:	/sys/miscfs/umapfs ${.CURDIR}/../..

NOMAN=	yep

KMOD=		umapfs
VFS_LKM=	yes

#
# These are not standard names, darnit!
MODVNOPS=umapfs_vnodeop_opv_desc
MODVFSOPS=umap_vfsops


SRCS=	lkminit_vfs.c
SRCS+=	umap_vfsops.c umap_vnops.c umap_subr.c

.include <bsd.kmod.mk>
