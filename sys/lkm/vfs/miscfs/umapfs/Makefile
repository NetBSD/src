#	$NetBSD: Makefile,v 1.3 1996/08/27 21:44:51 explorer Exp $

.include "../Makefile.inc"

.PATH:	$S/miscfs/umapfs ${.CURDIR}/../..

NOMAN=	yep

KMOD=		umapfs

SRCS=	lkminit_vfs.c
SRCS+=	umap_vfsops.c umap_vnops.c umap_subr.c

.include <bsd.kmod.mk>
