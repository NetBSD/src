#	$NetBSD: Makefile,v 1.5 1999/02/13 02:54:53 lukem Exp $

.include "../Makefile.inc"

.PATH:	$S/miscfs/umapfs ${.CURDIR}/../..

MKMAN=	no

KMOD=	umapfs

SRCS=	lkminit_vfs.c
SRCS+=	umap_vfsops.c umap_vnops.c umap_subr.c

.include <bsd.kmod.mk>
