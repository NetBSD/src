#	$NetBSD: Makefile,v 1.5.20.1 2002/01/08 00:33:30 nathanw Exp $

.include "../Makefile.inc"

.PATH:	$S/ufs/mfs ${.CURDIR}/../..

KMOD=	mfs

SRCS=	lkminit_vfs.c
SRCS+=	mfs_vfsops.c mfs_vnops.c

.include <bsd.kmod.mk>
