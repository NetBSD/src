#	$NetBSD: Makefile,v 1.10 2002/12/26 12:31:25 jdolecek Exp $

.include "../Makefile.inc"

.PATH:	$S/fs/msdosfs ${.CURDIR}/..

KMOD=	msdosfs

SRCS=	lkminit_vfs.c
SRCS+=	msdosfs_conv.c msdosfs_denode.c msdosfs_fat.c msdosfs_lookup.c
SRCS+=	msdosfs_vfsops.c msdosfs_vnops.c

.include <bsd.kmod.mk>
