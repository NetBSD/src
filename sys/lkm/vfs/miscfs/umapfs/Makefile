#	$NetBSD: Makefile,v 1.6 2001/12/12 01:50:17 tv Exp $

.include "../Makefile.inc"

.PATH:	$S/miscfs/umapfs ${.CURDIR}/../..

NOMAN=	# defined

KMOD=	umapfs

SRCS=	lkminit_vfs.c
SRCS+=	umap_vfsops.c umap_vnops.c umap_subr.c

.include <bsd.kmod.mk>
