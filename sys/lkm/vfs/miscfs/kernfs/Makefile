#	$NetBSD: Makefile,v 1.7 1999/02/12 05:41:58 cjs Exp $

.include "../Makefile.inc"

.PATH:	$S/miscfs/kernfs ${.CURDIR}/../..

MKMAN=	no

KMOD=		kernfs

SRCS=	lkminit_vfs.c
SRCS+=	kernfs_vfsops.c kernfs_vnops.c

.include <bsd.kmod.mk>
