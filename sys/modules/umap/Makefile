#	$NetBSD: Makefile,v 1.1.14.1 2011/05/31 03:05:06 rmind Exp $

.include "../Makefile.inc"

.PATH:	${S}/miscfs/umapfs

KMOD=	umap
SRCS=	umap_vfsops.c umap_vnops.c umap_subr.c

CWARNFLAGS.clang=	-Wno-conversion

.include <bsd.kmodule.mk>
