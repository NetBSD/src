#	$NetBSD: Makefile,v 1.3 1996/08/27 21:45:03 explorer Exp $

.include "../Makefile.inc"

.PATH:	$S/ufs/mfs ${.CURDIR}/../..

NOMAN=	yep

KMOD=		mfs

SRCS=	lkminit_vfs.c
SRCS+=	mfs_vfsops.c mfs_vnops.c

.include <bsd.kmod.mk>
