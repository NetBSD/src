#	$NetBSD: Makefile,v 1.1.8.2 2008/09/18 04:36:59 wrstuden Exp $

.include "../Makefile.inc"

.PATH:	${S}/miscfs/umapfs

KMOD=	umapfs
SRCS=	umap_vfsops.c umap_vnops.c umap_subr.c

.include <bsd.kmodule.mk>
