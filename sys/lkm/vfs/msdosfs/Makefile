#	$NetBSD: Makefile,v 1.7.20.2 2002/12/29 20:55:03 thorpej Exp $

.include "../Makefile.inc"

.PATH:	$S/fs/msdosfs ${.CURDIR}/..

KMOD=	msdosfs

SRCS=	lkminit_vfs.c
SRCS+=	msdosfs_conv.c msdosfs_denode.c msdosfs_fat.c msdosfs_lookup.c
SRCS+=	msdosfs_vfsops.c msdosfs_vnops.c

.include <bsd.kmod.mk>
