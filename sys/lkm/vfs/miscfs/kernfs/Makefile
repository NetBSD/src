#	$NetBSD: Makefile,v 1.2 1996/08/23 05:39:16 mrg Exp $

.include "../Makefile.inc"

.PATH:	$S/miscfs/kernfs ${.CURDIR}/../..

NOMAN=	yep

KMOD=		kernfs
VFS_LKM=	yes

SRCS=	lkminit_vfs.c
SRCS+=	kernfs_vfsops.c kernfs_vnops.c

.include <bsd.kmod.mk>
