#	$NetBSD: Makefile,v 1.1.10.2 2009/05/04 08:14:13 yamt Exp $

.include "../Makefile.inc"

.PATH:	${S}/miscfs/umapfs

KMOD=	umap
SRCS=	umap_vfsops.c umap_vnops.c umap_subr.c

.include <bsd.kmodule.mk>
