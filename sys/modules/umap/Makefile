#	$NetBSD: Makefile,v 1.3 2019/02/17 04:05:59 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/miscfs/umapfs

KMOD=	umap
SRCS=	umap_vfsops.c umap_vnops.c umap_subr.c

WARNS=	3

CWARNFLAGS.clang=	-Wno-conversion

.include <bsd.kmodule.mk>
