#	$NetBSD: Makefile,v 1.11 2003/09/08 06:51:59 itojun Exp $

.include "../Makefile.inc"

.PATH:	$S/miscfs/kernfs ${.CURDIR}/../..

KMOD=	kernfs

SRCS=	lkminit_vfs.c
SRCS+=	kernfs_vfsops.c kernfs_vnops.c kernfs_subr.c

.include <bsd.kmod.mk>
