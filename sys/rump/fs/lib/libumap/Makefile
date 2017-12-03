#	$NetBSD: Makefile,v 1.2.14.1 2017/12/03 11:39:13 jdolecek Exp $
#

.PATH:  ${.CURDIR}/../../../../miscfs/umapfs

LIB=	rumpfs_umap
COMMENT=uid/gid mapping layer

SRCS=	umap_subr.c umap_vfsops.c umap_vnops.c

CWARNFLAGS.clang+=	-Wno-conversion

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
