#	$NetBSD: Makefile,v 1.3 1996/08/27 21:44:55 explorer Exp $

.include "../Makefile.inc"

.PATH:	$S/msdosfs ${.CURDIR}/..

NOMAN=	yep

KMOD=		msdosfs

SRCS=	lkminit_vfs.c
SRCS+=	msdosfs_conv.c msdosfs_denode.c msdosfs_fat.c msdosfs_lookup.c
SRCS+=	msdosfs_vfsops.c msdosfs_vnops.c

.include <bsd.kmod.mk>
