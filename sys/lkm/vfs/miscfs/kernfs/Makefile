#	$NetBSD: Makefile,v 1.1 1996/08/22 20:18:38 explorer Exp $

.PATH:	/sys/miscfs/kernfs ${.CURDIR}/../..

NOMAN=	yep

KMOD=		kernfs
VFS_LKM=	yes

SRCS=	lkminit_vfs.c
SRCS+=	kernfs_vfsops.c kernfs_vnops.c

.include <bsd.kmod.mk>
