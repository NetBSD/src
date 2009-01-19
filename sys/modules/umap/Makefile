#	$NetBSD: Makefile,v 1.1.6.2 2009/01/19 13:20:11 skrll Exp $

.include "../Makefile.inc"

.PATH:	${S}/miscfs/umapfs

KMOD=	umap
SRCS=	umap_vfsops.c umap_vnops.c umap_subr.c

.include <bsd.kmodule.mk>
