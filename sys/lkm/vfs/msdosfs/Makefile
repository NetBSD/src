#	$NetBSD: Makefile,v 1.9 2001/12/12 12:06:49 lukem Exp $

.include "../Makefile.inc"

.PATH:	$S/msdosfs ${.CURDIR}/..

KMOD=	msdosfs

SRCS=	lkminit_vfs.c
SRCS+=	msdosfs_conv.c msdosfs_denode.c msdosfs_fat.c msdosfs_lookup.c
SRCS+=	msdosfs_vfsops.c msdosfs_vnops.c

.include <bsd.kmod.mk>
