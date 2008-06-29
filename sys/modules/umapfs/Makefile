#	$NetBSD: Makefile,v 1.1.2.2 2008/06/29 09:33:18 mjf Exp $

.include "../Makefile.inc"

.PATH:	${S}/miscfs/umapfs

KMOD=	umapfs
SRCS=	umap_vfsops.c umap_vnops.c umap_subr.c

.include <bsd.kmodule.mk>
