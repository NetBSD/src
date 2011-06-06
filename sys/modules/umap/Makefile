#	$NetBSD: Makefile,v 1.1.16.1 2011/06/06 09:09:50 jruoho Exp $

.include "../Makefile.inc"

.PATH:	${S}/miscfs/umapfs

KMOD=	umap
SRCS=	umap_vfsops.c umap_vnops.c umap_subr.c

CWARNFLAGS.clang=	-Wno-conversion

.include <bsd.kmodule.mk>
