#	$NetBSD: Makefile,v 1.1 1996/08/22 20:19:26 explorer Exp $

.PATH:	/sys/ufs/mfs ${.CURDIR}/../..

NOMAN=	yep

KMOD=		mfs
VFS_LKM=	yes

SRCS=	lkminit_vfs.c
SRCS+=	mfs_vfsops.c mfs_vnops.c

.include <bsd.kmod.mk>
