#	$NetBSD: Makefile,v 1.1.4.2 2009/01/17 13:29:30 mjf Exp $

.include "../Makefile.inc"

.PATH:	${S}/miscfs/umapfs

KMOD=	umap
SRCS=	umap_vfsops.c umap_vnops.c umap_subr.c

.include <bsd.kmodule.mk>
