#	$NetBSD: Makefile,v 1.1 1996/08/22 20:19:05 explorer Exp $

.PATH:	/sys/msdosfs ${.CURDIR}/..

NOMAN=	yep

KMOD=		msdosfs
VFS_LKM=	yes

SRCS=	lkminit_vfs.c
SRCS+=	msdosfs_conv.c msdosfs_denode.c msdosfs_fat.c msdosfs_lookup.c
SRCS+=	msdosfs_vfsops.c msdosfs_vnops.c

.include <bsd.kmod.mk>
