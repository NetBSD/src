#	$NetBSD: Makefile,v 1.5.22.1 2002/01/10 20:01:24 thorpej Exp $

.include "../Makefile.inc"

.PATH:	$S/miscfs/umapfs ${.CURDIR}/../..

KMOD=	umapfs

SRCS=	lkminit_vfs.c
SRCS+=	umap_vfsops.c umap_vnops.c umap_subr.c

.include <bsd.kmod.mk>
