#	$NetBSD: Makefile,v 1.5 1999/02/13 02:54:54 lukem Exp $

.include "../Makefile.inc"

.PATH:	$S/ufs/mfs ${.CURDIR}/../..

MKMAN=	no

KMOD=	mfs

SRCS=	lkminit_vfs.c
SRCS+=	mfs_vfsops.c mfs_vnops.c

.include <bsd.kmod.mk>
