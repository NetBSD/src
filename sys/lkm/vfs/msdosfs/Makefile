#	$NetBSD: Makefile,v 1.7.22.1 2002/01/10 20:01:26 thorpej Exp $

.include "../Makefile.inc"

.PATH:	$S/msdosfs ${.CURDIR}/..

KMOD=	msdosfs

SRCS=	lkminit_vfs.c
SRCS+=	msdosfs_conv.c msdosfs_denode.c msdosfs_fat.c msdosfs_lookup.c
SRCS+=	msdosfs_vfsops.c msdosfs_vnops.c

.include <bsd.kmod.mk>
