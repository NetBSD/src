#	$NetBSD: Makefile,v 1.1.4.2 2008/07/03 18:38:20 simonb Exp $

.include "../Makefile.inc"

.PATH:	${S}/miscfs/umapfs

KMOD=	umapfs
SRCS=	umap_vfsops.c umap_vnops.c umap_subr.c

.include <bsd.kmodule.mk>
