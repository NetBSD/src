#	$NetBSD: Makefile,v 1.2.56.1 2019/06/10 22:09:42 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/miscfs/umapfs

KMOD=	umap
SRCS=	umap_vfsops.c umap_vnops.c umap_subr.c

WARNS=	3

CWARNFLAGS.clang=	-Wno-conversion

.include <bsd.kmodule.mk>
