#	$NetBSD: Makefile,v 1.5.22.1 2002/01/10 20:01:30 thorpej Exp $

.include "../Makefile.inc"

.PATH:	$S/ufs/mfs ${.CURDIR}/../..

KMOD=	mfs

SRCS=	lkminit_vfs.c
SRCS+=	mfs_vfsops.c mfs_vnops.c

.include <bsd.kmod.mk>
