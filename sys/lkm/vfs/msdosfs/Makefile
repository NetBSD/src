#	$NetBSD: Makefile,v 1.7 1999/02/13 02:54:53 lukem Exp $

.include "../Makefile.inc"

.PATH:	$S/msdosfs ${.CURDIR}/..

MKMAN=	no

KMOD=	msdosfs

SRCS=	lkminit_vfs.c
SRCS+=	msdosfs_conv.c msdosfs_denode.c msdosfs_fat.c msdosfs_lookup.c
SRCS+=	msdosfs_vfsops.c msdosfs_vnops.c

.include <bsd.kmod.mk>
