#	$NetBSD: Makefile,v 1.8 2001/12/12 01:50:18 tv Exp $

.include "../Makefile.inc"

.PATH:	$S/msdosfs ${.CURDIR}/..

NOMAN=	# defined

KMOD=	msdosfs

SRCS=	lkminit_vfs.c
SRCS+=	msdosfs_conv.c msdosfs_denode.c msdosfs_fat.c msdosfs_lookup.c
SRCS+=	msdosfs_vfsops.c msdosfs_vnops.c

.include <bsd.kmod.mk>
