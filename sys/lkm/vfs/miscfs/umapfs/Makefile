#	$NetBSD: Makefile,v 1.4 1999/02/12 05:41:59 cjs Exp $

.include "../Makefile.inc"

.PATH:	$S/miscfs/umapfs ${.CURDIR}/../..

MKMAN=	no

KMOD=		umapfs

SRCS=	lkminit_vfs.c
SRCS+=	umap_vfsops.c umap_vnops.c umap_subr.c

.include <bsd.kmod.mk>
