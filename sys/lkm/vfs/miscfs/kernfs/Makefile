#	$NetBSD: Makefile,v 1.10.16.1 2004/08/03 10:54:04 skrll Exp $

.include "../Makefile.inc"

.PATH:	$S/miscfs/kernfs ${.CURDIR}/../..

KMOD=	kernfs

SRCS=	lkminit_vfs.c
SRCS+=	kernfs_vfsops.c kernfs_vnops.c kernfs_subr.c

.include <bsd.kmod.mk>
