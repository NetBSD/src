#	$NetBSD: Makefile,v 1.1.2.1 2011/06/06 09:10:06 jruoho Exp $
#

.PATH:  ${.CURDIR}/../../../../miscfs/umapfs

LIB=	rumpfs_umap

SRCS=	umap_subr.c umap_vfsops.c umap_vnops.c

CWARNFLAGS.clang+=	-Wno-conversion

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
