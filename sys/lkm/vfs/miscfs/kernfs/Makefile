#	$NetBSD: Makefile,v 1.12 2005/12/11 12:24:50 christos Exp $

.include "../Makefile.inc"

.PATH:	$S/miscfs/kernfs ${.CURDIR}/../..

KMOD=	kernfs

SRCS=	lkminit_vfs.c
SRCS+=	kernfs_vfsops.c kernfs_vnops.c kernfs_subr.c

.include <bsd.kmod.mk>
