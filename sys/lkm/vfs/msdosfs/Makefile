#	$NetBSD: Makefile,v 1.2 1996/08/23 05:39:26 mrg Exp $

.include "../Makefile.inc"

.PATH:	$S/msdosfs ${.CURDIR}/..

NOMAN=	yep

KMOD=		msdosfs
VFS_LKM=	yes

SRCS=	lkminit_vfs.c
SRCS+=	msdosfs_conv.c msdosfs_denode.c msdosfs_fat.c msdosfs_lookup.c
SRCS+=	msdosfs_vfsops.c msdosfs_vnops.c

.include <bsd.kmod.mk>
