#	$NetBSD: Makefile,v 1.6 1999/02/12 05:41:59 cjs Exp $

.include "../Makefile.inc"

.PATH:	$S/msdosfs ${.CURDIR}/..

MKMAN=	no

KMOD=		msdosfs

SRCS=	lkminit_vfs.c
SRCS+=	msdosfs_conv.c msdosfs_denode.c msdosfs_fat.c msdosfs_lookup.c
SRCS+=	msdosfs_vfsops.c msdosfs_vnops.c

.include <bsd.kmod.mk>
