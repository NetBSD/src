#	$NetBSD: Makefile,v 1.2 1996/08/23 05:39:33 mrg Exp $

.include "../Makefile.inc"

.PATH:	$S/ufs/mfs ${.CURDIR}/../..

NOMAN=	yep

KMOD=		mfs
VFS_LKM=	yes

SRCS=	lkminit_vfs.c
SRCS+=	mfs_vfsops.c mfs_vnops.c

.include <bsd.kmod.mk>
