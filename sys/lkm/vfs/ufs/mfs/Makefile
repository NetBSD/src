#	$NetBSD: Makefile,v 1.4 1999/02/12 05:42:00 cjs Exp $

.include "../Makefile.inc"

.PATH:	$S/ufs/mfs ${.CURDIR}/../..

MKMAN=	no

KMOD=		mfs

SRCS=	lkminit_vfs.c
SRCS+=	mfs_vfsops.c mfs_vnops.c

.include <bsd.kmod.mk>
